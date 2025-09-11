# ESP32-S3 checkm8 Quick Reference

## 🚀 Quick Start
1. **Flash firmware** to ESP32-S3 development board
2. **Connect to WiFi**: "ESP32S3-CheckM8" (password: "checkm8esp32")  
3. **Open browser**: http://192.168.4.1
4. **Connect device**: iOS device in DFU mode to USB-C port
5. **Start exploit**: Web interface or GPIO0 button

## 📋 Hardware Setup
```
ESP32-S3-DevKitC-1 or compatible board
├── GPIO2  → Status LED
├── GPIO0  → Manual trigger button (with pull-up)
├── GPIO21 → USB power control (optional)
├── USB0   → Primary USB-C (target device)
└── USB1   → Secondary USB-C (control/logging)
```

## 🌐 Web Interface
- **Dashboard**: http://192.168.4.1/
- **Status API**: http://192.168.4.1/api/status
- **Start Exploit**: POST to http://192.168.4.1/api/exploit
- **Configuration**: http://192.168.4.1/api/config

## 🔧 Build Commands
```bash
# PlatformIO (recommended)
cd esp32s3/
pio run -e esp32s3 -t upload

# Arduino CLI
../arduino-cli compile --fqbn esp32:esp32:esp32s3 checkm8_esp32s3.ino
```

## 🤖 Automation Examples

### Auto-exploit on device detection
```json
POST /api/config
{
  "automation": {
    "enabled": true,
    "auto_exploit": true
  }
}
```

### Scheduled exploitation every 30 seconds, 5 times
```json
POST /api/schedule
{
  "interval_ms": 30000,
  "repeat_count": 5,
  "auto_exploit": true
}
```

## 💡 Status LED Patterns
- **Off**: Idle, no device
- **Solid On**: Ready/Success
- **Slow Blink**: Exploit running
- **Fast Blink**: Error

## 🎯 Supported Devices
- **Apple A5**: iPhone 4S, iPad 2
- **Apple A5X**: iPad 2 (some variants)
- **Target chips**: S5L8940X, S5L8942X, S5L8945X
- **DFU mode**: VID:0x05AC, PID:0x1227

## 🔒 Security Notes
- Change default WiFi password in production
- Web interface has no authentication by default
- USB enumeration logs may contain device information
- Consider physical security for USB ports

## 🛠️ Troubleshooting
- **No device detected**: Check USB-C cable and DFU mode
- **Exploit fails**: Verify device compatibility and stable power
- **Web interface down**: Check WiFi connection and IP (192.168.4.1)
- **Build errors**: Ensure ESP32 board support and dependencies installed

## 📖 Documentation
- Full documentation: `esp32s3/README.md`
- Configuration examples: `esp32s3/config_example.json`
- Build scripts: `esp32s3/tools/`