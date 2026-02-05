#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>

// Color definitions
#define BLACK   0x0000
#define WHITE   0xFFFF
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define ORANGE  0xFD20
#define GRAY    0x8410
#define DARKGRAY 0x4208
#define LIGHTBLUE 0x5D9F

MCUFRIEND_kbv tft;

String buffer = "";
int screenWidth, screenHeight;
unsigned long lastDataTime = 0;
const unsigned long TIMEOUT = 10000; // 10 seconds timeout
bool welcomeShown = false;
bool screenLayoutDrawn = false;

// History arrays for mini graphs
float cpuHistory[30] = {0};
float ramHistory[30] = {0};
int historyIndex = 0;

void setup() {
  Serial.begin(9600);

  uint16_t ID = tft.readID();
  tft.begin(ID);

  tft.setRotation(2);  // Portrait mode (flipped)
  screenWidth = tft.width();
  screenHeight = tft.height();
  
  showWelcomeScreen();
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      welcomeShown = true;
      showData(buffer);
      buffer = "";
      lastDataTime = millis();
    } else {
      buffer += c;
    }
  }
  
  // Check if connection timeout
  if (welcomeShown && lastDataTime > 0 && (millis() - lastDataTime > TIMEOUT)) {
    showDisconnected();
    lastDataTime = 0; // Reset to avoid repeated calls
    screenLayoutDrawn = false; // Need to redraw layout after disconnect
  }
}

void drawBackground() {
  // Red gradient background (red to dark red)
  for (int y = 0; y < screenHeight; y++) {
    int redValue = 80 + (y * 100) / screenHeight; // 80 to 180
    uint16_t color = tft.color565(redValue, 0, 0);
    tft.drawFastHLine(0, y, screenWidth, color);
  }
}

void drawThumbsUpIcon(int x, int y, uint16_t color) {
  // Thumb
  tft.fillRect(x+15, y, 8, 15, color);
  // Palm
  tft.fillRect(x, y+15, 30, 20, color);
  // Fingers
  tft.fillRect(x, y+8, 8, 7, color);
  tft.fillRect(x+8, y+10, 7, 5, color);
  // Outline
  tft.drawRect(x, y+8, 8, 27, BLACK);
  tft.drawRect(x+8, y+10, 7, 25, BLACK);
  tft.drawRect(x+15, y, 8, 35, BLACK);
  tft.drawRect(x+23, y+15, 7, 20, BLACK);
}

void showWelcomeScreen() {
  // Nice gradient background (blue to purple)
  for (int y = 0; y < screenHeight; y++) {
    int blueValue = 50 + (y * 150) / screenHeight;
    int redValue = (y * 100) / screenHeight;
    uint16_t color = tft.color565(redValue, 0, blueValue);
    tft.drawFastHLine(0, y, screenWidth, color);
  }
  
  int centerX = screenWidth / 2;
  int centerY = screenHeight / 2;
  
  // Draw thumbs up icon
  drawThumbsUpIcon(centerX - 15, centerY - 60, GREEN);
  
  // Welcome text
  tft.setTextSize(3);
  tft.setTextColor(WHITE);
  tft.setCursor(centerX - 90, centerY + 10);
  tft.print("WELCOME!");
  
  tft.setTextSize(2);
  tft.setCursor(centerX - 110, centerY + 45);
  tft.print("System Monitor");
  
  tft.setTextSize(1);
  tft.setCursor(centerX - 65, centerY + 70);
  tft.print("Waiting for data...");
  
  // Decorative border
  tft.drawRect(5, 5, screenWidth - 10, screenHeight - 10, CYAN);
  tft.drawRect(6, 6, screenWidth - 12, screenHeight - 12, CYAN);
  
  screenLayoutDrawn = false; // Reset flag when showing welcome
}

void drawDisconnectedIcon(int x, int y, uint16_t color) {
  // Draw WiFi/connection icon with X
  tft.drawCircle(x+15, y+15, 5, color);
  tft.drawCircle(x+15, y+15, 10, color);
  tft.drawCircle(x+15, y+15, 15, color);
  // Draw X over it
  tft.drawLine(x+5, y+5, x+25, y+25, RED);
  tft.drawLine(x+25, y+5, x+5, y+25, RED);
}

void showDisconnected() {
  drawBackground();
  
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  
  int centerX = screenWidth / 2;
  int centerY = screenHeight / 2;
  
  drawDisconnectedIcon(centerX - 15, centerY - 50, WHITE);
  
  tft.setCursor(centerX - 100, centerY + 20);
  tft.print("NOT CONNECTED");
  
  tft.setTextSize(2);
  tft.setCursor(centerX - 80, centerY + 55);
  tft.print("Check Data Sender");
  
  screenLayoutDrawn = false; // Reset flag when showing disconnect
}

void drawCPUIcon(int x, int y, uint16_t color) {
  tft.drawRect(x, y, 20, 20, color);
  tft.drawRect(x+2, y+2, 16, 16, color);
  tft.drawFastHLine(x+6, y+8, 8, color);
  tft.drawFastHLine(x+6, y+12, 8, color);
}

void drawRAMIcon(int x, int y, uint16_t color) {
  tft.drawRect(x, y+4, 20, 12, color);
  for (int i = 0; i < 4; i++) {
    tft.drawFastVLine(x + 3 + i*4, y, 4, color);
  }
}

void drawNetworkIcon(int x, int y, uint16_t color) {
  tft.drawCircle(x+10, y+10, 3, color);
  tft.drawCircle(x+10, y+10, 7, color);
  tft.drawCircle(x+10, y+10, 10, color);
}

void drawTempIcon(int x, int y, uint16_t color) {
  tft.drawCircle(x+10, y+4, 3, color);
  tft.drawRect(x+7, y+7, 6, 10, color);
  tft.fillRect(x+8, y+13, 4, 4, color);
}

void drawBatteryIcon(int x, int y, uint16_t color) {
  // Battery body
  tft.drawRect(x, y+2, 16, 10, color);
  // Battery tip
  tft.fillRect(x+16, y+5, 2, 4, color);
  // Battery fill indicator
  tft.fillRect(x+2, y+4, 4, 6, color);
}

void drawDiskIcon(int x, int y, uint16_t color) {
  // Disk platter
  tft.drawCircle(x+8, y+8, 7, color);
  tft.drawCircle(x+8, y+8, 3, color);
  // Disk lines
  tft.drawFastHLine(x+4, y+8, 8, color);
}

void drawFanIcon(int x, int y, uint16_t color) {
  // Fan center
  tft.fillCircle(x+10, y+8, 2, color);
  // Fan blades
  tft.fillTriangle(x+10, y+8, x+6, y+2, x+8, y+6, color);
  tft.fillTriangle(x+10, y+8, x+14, y+2, x+12, y+6, color);
  tft.fillTriangle(x+10, y+8, x+16, y+10, x+12, y+9, color);
  tft.fillTriangle(x+10, y+8, x+4, y+10, x+8, y+9, color);
}

void drawWeatherIcon(int x, int y, int weatherId) {
  if (weatherId >= 200 && weatherId < 300) {
    // Thunderstorm - lightning bolt
    tft.fillTriangle(x+8, y, x+12, y+8, x+10, y+8, YELLOW);
    tft.fillTriangle(x+10, y+8, x+12, y+8, x+6, y+16, YELLOW);
  } else if (weatherId >= 300 && weatherId < 600) {
    // Rain - raindrops
    for (int i = 0; i < 3; i++) {
      tft.fillCircle(x + 5 + i*5, y + 8 + (i%2)*4, 2, CYAN);
    }
  } else if (weatherId >= 600 && weatherId < 700) {
    // Snow - snowflake
    tft.drawFastHLine(x+4, y+8, 12, WHITE);
    tft.drawFastVLine(x+10, y+2, 12, WHITE);
    tft.drawLine(x+6, y+4, x+14, y+12, WHITE);
    tft.drawLine(x+14, y+4, x+6, y+12, WHITE);
  } else if (weatherId >= 800 && weatherId < 900) {
    if (weatherId == 800) {
      // Clear - sun
      tft.fillCircle(x+10, y+8, 5, YELLOW);
      for (int i = 0; i < 8; i++) {
        float angle = i * 45 * 3.14159 / 180;
        int x1 = x + 10 + cos(angle) * 7;
        int y1 = y + 8 + sin(angle) * 7;
        int x2 = x + 10 + cos(angle) * 9;
        int y2 = y + 8 + sin(angle) * 9;
        tft.drawLine(x1, y1, x2, y2, YELLOW);
      }
    } else {
      // Clouds
      tft.fillCircle(x+6, y+10, 4, WHITE);
      tft.fillCircle(x+10, y+8, 5, WHITE);
      tft.fillCircle(x+14, y+10, 4, WHITE);
    }
  }
}

void drawProgressBar(int x, int y, int w, int h, float percent, uint16_t color, String label) {
  // Clear the bar area first (except border)
  tft.fillRect(x+1, y+1, w-2, h-2, tft.color565(120, 0, 0));
  
  // Draw outer border
  tft.drawRect(x, y, w, h, WHITE);
  
  // Fill progress
  int fillWidth = (w - 2) * percent / 100;
  if (fillWidth > 0) {
    tft.fillRect(x+1, y+1, fillWidth, h-2, color);
  }
  
  // Draw percentage text inside the bar
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  
  // Calculate text position to center it
  String percentText = String((int)percent) + "%";
  int textWidth = percentText.length() * 12; // Approximate width
  int textX = x + (w - textWidth) / 2;
  int textY = y + (h - 16) / 2; // Center vertically (16 is text height)
  
  tft.setCursor(textX, textY);
  tft.print(percentText);
}

void showData(String data) {
  data.trim();

  // Parse: cpu,ram,net_down,net_up,weather_temp,weather_id,cpu_temp,gpu_temp,battery_level,disk_usage,cpu_freq,uptime,ping,date,time
  int indices[14];
  indices[0] = data.indexOf(',');
  for (int i = 1; i < 14; i++) {
    indices[i] = data.indexOf(',', indices[i-1] + 1);
  }

  if (indices[13] < 0) return; // invalid data

  float cpu = data.substring(0, indices[0]).toFloat();
  float ram = data.substring(indices[0] + 1, indices[1]).toFloat();
  float netDown = data.substring(indices[1] + 1, indices[2]).toFloat();
  float netUp = data.substring(indices[2] + 1, indices[3]).toFloat();
  float weatherTemp = data.substring(indices[3] + 1, indices[4]).toFloat();
  int weatherId = data.substring(indices[4] + 1, indices[5]).toInt();
  float cpuTemp = data.substring(indices[5] + 1, indices[6]).toFloat();
  float gpuTemp = data.substring(indices[6] + 1, indices[7]).toFloat();
  float batteryLevel = data.substring(indices[7] + 1, indices[8]).toFloat();
  float diskUsage = data.substring(indices[8] + 1, indices[9]).toFloat();
  float cpuFreq = data.substring(indices[9] + 1, indices[10]).toFloat();
  String bootTime = data.substring(indices[10] + 1, indices[11]);
  int ping = data.substring(indices[11] + 1, indices[12]).toInt();
  String shamsiDate = data.substring(indices[12] + 1, indices[13]);
  String shamsiTime = data.substring(indices[13] + 1);
  
  // Store CPU and RAM history for graphs
  cpuHistory[historyIndex] = cpu;
  ramHistory[historyIndex] = ram;
  historyIndex = (historyIndex + 1) % 30;

  // Only draw background and layout on first data or after disconnect
  if (!screenLayoutDrawn) {
    drawBackground();
    
    // Decorative top bar (taller for weather)
    tft.fillRect(0, 0, screenWidth, 35, tft.color565(100, 0, 0));
    
    // Draw static elements (icons and labels) for portrait
    int yPos = 45;
    
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    
    // CPU section with icon inside bar area
    tft.setCursor(75, yPos + 2);
    yPos += 18;
    
    // RAM section with icon inside bar area
    tft.setCursor(75, yPos + 2);
    yPos += 23;
    
    // Network section
    yPos += 20;
    
    tft.fillTriangle(10, yPos+8, 5, yPos, 15, yPos, GREEN);
    tft.setCursor(20, yPos + 3);
    tft.print("DOWNLOAD");
    yPos += 20;
    
    tft.fillTriangle(10, yPos, 5, yPos+8, 15, yPos+8, ORANGE);
    tft.setCursor(20, yPos + 3);
    tft.print("UPLOAD");
    yPos += 23;
    
    // System info section
    yPos += 5;
    
    drawDiskIcon(5, yPos, MAGENTA);
    tft.setCursor(25, yPos + 3);
    tft.print("DISK");
    yPos += 18;
    
    drawBatteryIcon(5, yPos, GREEN);
    tft.setCursor(25, yPos + 3);
    tft.print("BAT");
    yPos += 18;
    
    drawTempIcon(5, yPos, ORANGE);
    tft.setCursor(25, yPos + 3);
    tft.print("CPU");
    yPos += 18;
    
    drawTempIcon(5, yPos, RED);
    tft.setCursor(25, yPos + 3);
    tft.print("GPU");
    yPos += 18;
    
    tft.fillCircle(10, yPos+5, 2, CYAN);
    tft.setCursor(25, yPos + 3);
    tft.print("FREQ");
    yPos += 18;
    
    tft.fillRect(5, yPos+3, 3, 8, YELLOW);
    tft.fillRect(9, yPos+5, 3, 6, YELLOW);
    tft.fillRect(13, yPos+7, 3, 4, YELLOW);
    tft.setCursor(25, yPos + 3);
    tft.print("UP");
    yPos += 18;
    
    tft.fillTriangle(10, yPos+3, 5, yPos+10, 15, yPos+10, GREEN);
    tft.setCursor(25, yPos + 3);
    tft.print("PING");
    
    screenLayoutDrawn = true;
  }
  
  // Update dynamic data only
  // Clear and update date/time area
  tft.fillRect(10, 5, 160, 30, tft.color565(100, 0, 0));

  // Header - Date on left
  tft.setTextSize(2);
  tft.setTextColor(YELLOW);
  tft.setCursor(10, 5);
  tft.print(shamsiDate);
  
  // Time in center-left
  tft.setTextColor(CYAN);
  tft.setCursor(10, 20);
  tft.print(shamsiTime);

  // Clear and update weather section
  int weatherX = screenWidth - 95;
  tft.fillRect(weatherX, 5, 85, 25, tft.color565(100, 0, 0));
  tft.setTextSize(2);
  tft.setTextColor(YELLOW);
  tft.setCursor(weatherX, 8);
  tft.print(weatherTemp, 1);
  tft.setTextSize(1);
  tft.print("C");
  drawWeatherIcon(weatherX + 55, 5, weatherId);

  // Update dynamic data for portrait with smaller progress bars
  int yPos = 45;
  int barWidth = screenWidth - 10;
  int barHeight = 18;
  
  // CPU progress bar with icon
  drawCPUIcon(5, yPos, GREEN);
  drawProgressBar(30, yPos, barWidth - 25, barHeight, cpu, GREEN, "CPU");
  yPos += 24;

  // RAM progress bar with icon
  drawRAMIcon(5, yPos, CYAN);
  drawProgressBar(30, yPos, barWidth - 25, barHeight, ram, CYAN, "RAM");
  yPos += 29;

  // Network download - value on right
  yPos += 5;
  tft.fillRect(90, yPos + 3, screenWidth - 95, 10, tft.color565(120, 0, 0));
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  // Right align the value
  String downText = "";
  if (netDown >= 1000) {
    downText = String(netDown / 1024, 1) + " MB/s";
  } else {
    downText = String(netDown, 1) + " KB/s";
  }
  int textWidth = downText.length() * 6;
  tft.setCursor(screenWidth - textWidth - 5, yPos + 3);
  tft.print(downText);
  yPos += 20;

  // Network upload - value on right
  tft.fillRect(90, yPos + 3, screenWidth - 95, 10, tft.color565(120, 0, 0));
  String upText = "";
  if (netUp >= 1000) {
    upText = String(netUp / 1024, 1) + " MB/s";
  } else {
    upText = String(netUp, 1) + " KB/s";
  }
  textWidth = upText.length() * 6;
  tft.setCursor(screenWidth - textWidth - 5, yPos + 3);
  tft.print(upText);
  yPos += 23;

  // Battery level - value on right
  yPos += 5;
  tft.fillRect(55, yPos + 3, screenWidth - 60, 10, tft.color565(120, 0, 0));
  String diskText = String(diskUsage, 1) + " %";
  textWidth = diskText.length() * 6;
  tft.setCursor(screenWidth - textWidth - 5, yPos + 3);
  tft.print(diskText);
  yPos += 18;
  
  tft.fillRect(55, yPos + 3, screenWidth - 60, 10, tft.color565(120, 0, 0));
  String batText = (batteryLevel > 0) ? String(batteryLevel, 1) + " %" : "N/A";
  textWidth = batText.length() * 6;
  tft.setCursor(screenWidth - textWidth - 5, yPos + 3);
  tft.print(batText);
  yPos += 18;

  // CPU temperature - value on right
  tft.fillRect(55, yPos + 3, screenWidth - 60, 10, tft.color565(120, 0, 0));
  String cpuTempText = (cpuTemp > 0) ? String(cpuTemp, 1) + " C" : "N/A";
  textWidth = cpuTempText.length() * 6;
  tft.setCursor(screenWidth - textWidth - 5, yPos + 3);
  tft.print(cpuTempText);
  yPos += 18;

  // GPU temperature - value on right
  tft.fillRect(55, yPos + 3, screenWidth - 60, 10, tft.color565(120, 0, 0));
  String gpuTempText = (gpuTemp > 0) ? String(gpuTemp, 1) + " C" : "N/A";
  textWidth = gpuTempText.length() * 6;
  tft.setCursor(screenWidth - textWidth - 5, yPos + 3);
  tft.print(gpuTempText);
  yPos += 18;
  
  // CPU Frequency
  tft.fillRect(55, yPos + 3, screenWidth - 60, 10, tft.color565(120, 0, 0));
  String freqText = (cpuFreq > 0) ? String(cpuFreq, 2) + " GHz" : "N/A";
  textWidth = freqText.length() * 6;
  tft.setCursor(screenWidth - textWidth - 5, yPos + 3);
  tft.print(freqText);
  yPos += 18;
  
  // Boot Time
  tft.fillRect(50, yPos + 3, screenWidth - 55, 10, tft.color565(120, 0, 0));
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  // Right align - use fixed position since length calculation doesn't work well with Persian
  tft.setCursor(screenWidth - (bootTime.length() * 6) - 5, yPos + 3);
  tft.print(bootTime);
  yPos += 18;
  
  // Ping
  tft.fillRect(55, yPos + 3, screenWidth - 60, 10, tft.color565(120, 0, 0));
  String pingText = (ping > 0) ? String(ping) + " ms" : "N/A";
  textWidth = pingText.length() * 6;
  tft.setCursor(screenWidth - textWidth - 5, yPos + 3);
  tft.print(pingText);
}
