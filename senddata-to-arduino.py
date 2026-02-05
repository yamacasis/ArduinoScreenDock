import psutil
import serial
import time
import requests
import jdatetime
import subprocess

PORT = "COM7"   # Change this!
BAUD = 9600

ser = serial.Serial(PORT, BAUD)
time.sleep(2)

# Network speed tracking
prev_net_down = 0
prev_net_up = 0
prev_time = time.time()

# Uptime base caching (avoid expensive Windows event log query every loop)
_uptime_base_epoch = None
_uptime_base_last_refresh = 0.0


def _get_windows_last_start_or_wake_epoch():
    """Return unix epoch seconds of the most recent system start or wake event."""
    ps = (
        "$boot = (Get-WinEvent -FilterHashtable @{LogName='System'; ProviderName='Microsoft-Windows-Kernel-General'; Id=12} -MaxEvents 1).TimeCreated;"
        "$wake = (Get-WinEvent -FilterHashtable @{LogName='System'; ProviderName='Microsoft-Windows-Power-Troubleshooter'; Id=1} -MaxEvents 1).TimeCreated;"
        "$t = $boot; if ($wake -and $boot -and $wake -gt $boot) { $t = $wake };"
        "if (-not $t) { $t = (Get-CimInstance Win32_OperatingSystem).LastBootUpTime };"
        "[int64]([DateTimeOffset]$t).ToUnixTimeSeconds()"
    )
    out = subprocess.check_output(
        ['powershell', '-NoProfile', '-Command', ps],
        timeout=5,
        stderr=subprocess.DEVNULL,
    ).decode(errors='ignore').strip()
    return int(out)


def get_weather():
    API_KEY = "1921df3023e1c648e05ba2ded0aab4d1"
    CITY = "Tehran"

    url = f"https://api.openweathermap.org/data/2.5/weather?q={CITY}&appid={API_KEY}&units=metric"

    r = requests.get(url).json()
    temp = r["main"]["temp"]
    weather_id = r["weather"][0]["id"]
    return temp, weather_id


def get_cpu_temp():
    """Get CPU temperature using various methods"""
    # Method 1: Try psutil sensors (works on Linux)
    try:
        if hasattr(psutil, 'sensors_temperatures'):
            temps = psutil.sensors_temperatures()
            if temps:
                print(f"Available temperature sensors: {list(temps.keys())}")
                
                if 'coretemp' in temps:
                    return round(temps['coretemp'][0].current, 1)
                elif 'cpu_thermal' in temps:
                    return round(temps['cpu_thermal'][0].current, 1)
                elif 'k10temp' in temps:  # AMD
                    return round(temps['k10temp'][0].current, 1)
                elif 'acpitz' in temps:
                    return round(temps['acpitz'][0].current, 1)
                # Get first available temp sensor
                for name, entries in temps.items():
                    if entries:
                        print(f"Using sensor: {name} = {entries[0].current}°C")
                        return round(entries[0].current, 1)
    except Exception as e:
        pass  # Silent fail on Windows
    
    # Method 2: Try Windows WMI directly with OpenHardwareMonitor
    try:
        import wmi
        import pythoncom
        pythoncom.CoInitialize()
        try:
            w = wmi.WMI(namespace="root\\OpenHardwareMonitor")
            temperature_infos = w.Sensor()
            for sensor in temperature_infos:
                if sensor.SensorType == 'Temperature' and ('CPU' in sensor.Name or 'Core' in sensor.Name):
                    temp = round(sensor.Value, 1)
                    print(f"OpenHardwareMonitor: {sensor.Name} = {temp}°C")
                    return temp
        finally:
            pythoncom.CoUninitialize()
    except Exception as e:
        pass  # Silent fail, try next method
    
    # Method 3: Try LibreHardwareMonitor
    try:
        import wmi
        import pythoncom
        pythoncom.CoInitialize()
        try:
            w = wmi.WMI(namespace="root\\LibreHardwareMonitor")
            temperature_infos = w.Sensor()
            for sensor in temperature_infos:
                if sensor.SensorType == 'Temperature' and ('CPU' in sensor.Name or 'Core' in sensor.Name):
                    temp = round(sensor.Value, 1)
                    print(f"LibreHardwareMonitor: {sensor.Name} = {temp}°C")
                    return temp
        finally:
            pythoncom.CoUninitialize()
    except Exception as e:
        pass  # Silent fail, try next method
    
    # Method 4: Try Windows Thermal Zone via PowerShell
    try:
        import subprocess
        result = subprocess.check_output(
            ['powershell', '-Command',
             "(Get-CimInstance -Namespace root/wmi -ClassName MSAcpi_ThermalZoneTemperature | Select-Object -First 1).CurrentTemperature"],
            timeout=3, stderr=subprocess.DEVNULL
        ).decode().strip()
        if result and result.isdigit():
            temp = (int(result) - 2732) / 10.0  # Convert from decikelvin to Celsius
            if 0 < temp < 150:
                print(f"WMI Thermal Zone: {temp}°C")
                return round(temp, 1)
    except Exception as e:
        pass  # Silent fail
    
    # No temperature sensor found - print message once every 60 seconds
    import time
    current_time = time.time()
    if not hasattr(get_cpu_temp, 'last_warning') or current_time - get_cpu_temp.last_warning > 60:
        print("\n*** CPU TEMPERATURE NOT AVAILABLE ***")
        print("To enable CPU temperature monitoring:")
        print("1. Download LibreHardwareMonitor: https://github.com/LibreHardwareMonitor/LibreHardwareMonitor/releases")
        print("2. Run it as Administrator (right-click -> Run as administrator)")
        print("3. Keep it running in the background\n")
        get_cpu_temp.last_warning = current_time
    
    return 0


def get_gpu_temp():
    """Get GPU temperature for NVIDIA, AMD, or Intel GPUs"""
    # Try NVIDIA GPU via nvidia-smi
    try:
        import subprocess
        result = subprocess.check_output(
            ['nvidia-smi', '--query-gpu=temperature.gpu', '--format=csv,noheader,nounits'],
            timeout=2, stderr=subprocess.DEVNULL
        ).decode().strip()
        if result:
            temp = round(float(result), 1)
            return temp
    except Exception as e:
        pass  # Silent fail, try next method
    
    # Try OpenHardwareMonitor via WMI
    try:
        import wmi
        import pythoncom
        pythoncom.CoInitialize()
        try:
            w = wmi.WMI(namespace="root\\OpenHardwareMonitor")
            temperature_infos = w.Sensor()
            for sensor in temperature_infos:
                if sensor.SensorType == 'Temperature' and 'GPU' in sensor.Name:
                    temp = round(sensor.Value, 1)
                    print(f"OpenHardwareMonitor GPU: {sensor.Name} = {temp}°C")
                    return temp
        finally:
            pythoncom.CoUninitialize()
    except Exception as e:
        pass  # Silent fail, try next method
    
    # Try LibreHardwareMonitor via WMI
    try:
        import wmi
        import pythoncom
        pythoncom.CoInitialize()
        try:
            w = wmi.WMI(namespace="root\\LibreHardwareMonitor")
            temperature_infos = w.Sensor()
            for sensor in temperature_infos:
                if sensor.SensorType == 'Temperature' and 'GPU' in sensor.Name:
                    temp = round(sensor.Value, 1)
                    print(f"LibreHardwareMonitor GPU: {sensor.Name} = {temp}°C")
                    return temp
        finally:
            pythoncom.CoUninitialize()
    except Exception as e:
        pass  # Silent fail
    
    return 0


def get_battery_level():
    """Get battery percentage"""
    try:
        battery = psutil.sensors_battery()
        if battery:
            return round(battery.percent, 1)
    except:
        pass
    return 0


def get_disk_usage():
    """Get C: drive usage percentage"""
    try:
        disk = psutil.disk_usage('C:\\')
        return round(disk.percent, 1)
    except:
        return 0


def get_cpu_freq():
    """Get current CPU frequency in GHz"""
    try:
        freq = psutil.cpu_freq()
        if freq:
            return round(freq.current / 1000, 2)  # Convert MHz to GHz
    except:
        pass
    return 0


def get_uptime():
    """Get system uptime (time since boot) as a compact string.

    Format:
      - HH:MM            (for < 1 day)
      - Xd HH:MM         (for >= 1 day)

    Note: Must not contain commas because the Arduino side parses CSV.
    """
    try:
        global _uptime_base_epoch, _uptime_base_last_refresh
        now = time.time()

        # Refresh at most every 5 minutes (and also when base is unknown)
        if _uptime_base_epoch is None or (now - _uptime_base_last_refresh) > 300:
            base = None
            try:
                base = _get_windows_last_start_or_wake_epoch()
            except Exception:
                base = int(psutil.boot_time())
            _uptime_base_epoch = int(base)
            _uptime_base_last_refresh = now

        uptime_seconds = max(0, int(now - _uptime_base_epoch))

        days, rem = divmod(uptime_seconds, 86400)
        hours, rem = divmod(rem, 3600)
        minutes, seconds = divmod(rem, 60)

        if days > 0:
            return f"{days}d {hours:02}:{minutes:02}"
        return f"{hours:02}:{minutes:02}"
    except Exception as e:
        print(f"Uptime error: {e}")
        return "00:00"


def get_ping():
    """Get ping to Google DNS (8.8.8.8) in milliseconds"""
    try:
        import subprocess
        result = subprocess.check_output(
            ['ping', '-n', '1', '8.8.8.8'],
            timeout=2,
            stderr=subprocess.DEVNULL
        ).decode()
        # Extract time from ping result
        if 'time=' in result or 'time<' in result:
            for line in result.split('\n'):
                if 'time' in line.lower():
                    if 'time=' in line:
                        time_str = line.split('time=')[1].split('ms')[0]
                        return round(float(time_str), 0)
                    elif 'time<' in line:
                        return 1  # Very fast ping
    except:
        pass
    return 0


def get_fan_speed():
    """Get fan speed (RPM) if available"""
    # Note: Requires OpenHardwareMonitor or LibreHardwareMonitor running as Administrator
    
    # Try OpenHardwareMonitor via WMI
    try:
        import wmi
        import pythoncom
        pythoncom.CoInitialize()
        try:
            w = wmi.WMI(namespace="root\\OpenHardwareMonitor")
            sensor_infos = w.Sensor()
            for sensor in sensor_infos:
                if sensor.SensorType == 'Fan':
                    rpm = round(sensor.Value, 0)
                    print(f"OpenHardwareMonitor Fan: {sensor.Name} = {rpm} RPM")
                    return rpm
        finally:
            pythoncom.CoUninitialize()
    except Exception as e:
        pass  # Silent fail, try next method
    
    # Try LibreHardwareMonitor via WMI
    try:
        import wmi
        import pythoncom
        pythoncom.CoInitialize()
        try:
            w = wmi.WMI(namespace="root\\LibreHardwareMonitor")
            sensor_infos = w.Sensor()
            for sensor in sensor_infos:
                if sensor.SensorType == 'Fan':
                    rpm = round(sensor.Value, 0)
                    print(f"LibreHardwareMonitor Fan: {sensor.Name} = {rpm} RPM")
                    return rpm
        finally:
            pythoncom.CoUninitialize()
    except Exception as e:
        pass  # Silent fail
    
    # No hardware monitor found - print message once every 60 seconds
    import time
    current_time = time.time()
    if not hasattr(get_fan_speed, 'last_warning') or current_time - get_fan_speed.last_warning > 60:
        print("\n*** FAN SPEED NOT AVAILABLE ***")
        print("To enable fan speed monitoring:")
        print("1. Download LibreHardwareMonitor: https://github.com/LibreHardwareMonitor/LibreHardwareMonitor/releases")
        print("2. Run it as Administrator (right-click -> Run as administrator)")
        print("3. Keep it running in the background\n")
        get_fan_speed.last_warning = current_time
    
    return 0


while True:

    cpu = psutil.cpu_percent()
    ram = psutil.virtual_memory().percent
    
    # Calculate network speed (download/upload in KB/s)
    net_io = psutil.net_io_counters()
    current_time = time.time()
    time_diff = current_time - prev_time

    # If the machine slept/hibernated, time_diff will be huge.
    # Reset network speed baselines and force uptime base refresh.
    if time_diff > 30:
        prev_net_down = net_io.bytes_recv
        prev_net_up = net_io.bytes_sent
        prev_time = current_time
        time_diff = 1.0
        _uptime_base_epoch = None
    
    net_down = (net_io.bytes_recv - prev_net_down) / 1024 / time_diff  # KB/s
    net_up = (net_io.bytes_sent - prev_net_up) / 1024 / time_diff      # KB/s
    
    prev_net_down = net_io.bytes_recv
    prev_net_up = net_io.bytes_sent
    prev_time = current_time

    try:
        weather_temp, weather_id = get_weather()
    except:
        weather_temp = 0
        weather_id = 800

    cpu_temp = get_cpu_temp()
    gpu_temp = get_gpu_temp()
    battery_level = get_battery_level()
    disk_usage = get_disk_usage()
    cpu_freq = get_cpu_freq()
    uptime = get_uptime()
    ping = get_ping()

    # Get Shamsi (Jalali) date and time
    now = jdatetime.datetime.now()
    shamsi_date = now.strftime("%Y/%m/%d")
    shamsi_time = now.strftime("%H:%M")

    # Add day of week name (English abbreviations for Arduino compatibility)
    day_names = ['SH', '1SH', '2SH', '3SH', '$SH', '5SH', 'JOM']
    day_name = day_names[now.weekday()]
    
    shamsi_time = shamsi_time + " " + day_name


    # Format: cpu,ram,net_down,net_up,weather_temp,weather_id,cpu_temp,gpu_temp,battery_level,disk_usage,cpu_freq,uptime,ping,date,time
    msg = f"{cpu},{ram},{net_down:.1f},{net_up:.1f},{weather_temp},{weather_id},{cpu_temp},{gpu_temp},{battery_level},{disk_usage},{cpu_freq},{uptime},{ping},{shamsi_date},{shamsi_time}\n"

    ser.write(msg.encode())

    print("Sent:", msg)

    time.sleep(3)
