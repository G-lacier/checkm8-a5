# ESP32-S3 Enhanced checkm8 Implementation

## Overview

This is a comprehensive port of the checkm8 exploit to the ESP32-S3 microcontroller with dual USB-C support. Unlike the original Arduino+MAX3421E implementation, this version is fully self-contained and automation-ready.

## Key Features

### 🔧 Full Containment
- **No External MCU Required**: All functionality runs natively on ESP32-S3
- **Native USB Host**: Uses ESP32-S3's built-in USB capabilities instead of external shield
- **Self-Contained Operation**: Complete exploit implementation without dependencies

### 🔌 Dual USB-C Support
- **USB Port 0**: Primary device detection and exploitation
- **USB Port 1**: Secondary functions (logging, control interface)
- **Role Switching**: Automatic USB host/device/OTG role detection
- **Power Management**: Intelligent USB power control

### 🤖 Automation Features
- **FreeRTOS Tasks**: Multi-threaded operation with dedicated USB, automation, and web tasks
- **Web Interface**: Complete REST API and web dashboard for remote control
- **GPIO Triggers**: Hardware button for manual exploit triggering
- **Scheduled Automation**: Configurable timers and repeat counts
- **Auto-Detection**: Automatic device detection and exploitation

### 🌐 Web Control Interface
- **WiFi AP Mode**: Creates isolated network for secure control
- **Real-Time Status**: Live monitoring of exploit progress and device status
- **Configuration Management**: Web-based settings for automation and USB ports
- **API Endpoints**: RESTful API for integration and scripting

## Hardware Requirements

### ESP32-S3 Development Board
- **Recommended**: ESP32-S3-DevKitC-1 or compatible
- **Flash**: Minimum 4MB (8MB+ recommended)
- **PSRAM**: Optional but recommended for enhanced performance
- **Dual USB**: Native USB-C ports on ESP32-S3-WROOM-1

### GPIO Connections
```
GPIO2  - Status LED (built-in LED on most dev boards)
GPIO0  - Manual trigger button (with pull-up resistor)
GPIO21 - USB power control (optional, for power switching)
```

### USB-C Connectors
- **Port 0**: Primary USB-C connector for target device
- **Port 1**: Secondary USB-C connector for control/logging

## Installation & Setup

### Prerequisites
- [PlatformIO](https://platformio.org/) or Arduino IDE with ESP32 support
- ESP32-S3 board definitions
- Required libraries (automatically installed)

### Building with PlatformIO
```bash
# Clone the repository
git clone https://github.com/G-lacier/checkm8-a5.git
cd checkm8-a5/esp32s3

# Build the project
pio run -e esp32s3

# Upload to device
pio run -e esp32s3 -t upload

# Monitor serial output
pio device monitor
```

### Building with Arduino IDE
1. Install ESP32 board support: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
2. Select board: "ESP32S3 Dev Module"
3. Install required libraries:
   - ArduinoJson (6.21.3+)
4. Open `esp32s3/checkm8_esp32s3.ino`
5. Compile and upload

## Usage

### Initial Setup
1. **Flash Firmware**: Upload the compiled firmware to ESP32-S3
2. **Power On**: Connect ESP32-S3 to power source
3. **Connect to WiFi**: Join "ESP32S3-CheckM8" network (password: "checkm8esp32")
4. **Open Web Interface**: Navigate to http://192.168.4.1

### Basic Operation
1. **Connect Target Device**: Put iOS device in DFU mode and connect to USB-C port
2. **Start Exploit**: Use web interface or press GPIO0 button
3. **Monitor Progress**: Watch status LED and web interface for exploit progress
4. **Complete**: LED will indicate success/failure

### Advanced Automation
```json
{
  "automation": {
    "enabled": true,
    "interval_ms": 30000,
    "repeat_count": 5,
    "auto_exploit": true
  }
}
```

## Web API Reference

### GET /api/status
Returns current system status including exploit state, connected devices, and automation configuration.

### POST /api/exploit
Triggers the checkm8 exploit manually. Returns success/failure status.

### GET/POST /api/config
Retrieves or updates system configuration including automation settings and USB port modes.

### POST /api/reset
Resets the system state and stops any running automation.

### GET /api/devices
Lists all connected USB devices with their VID/PID and DFU status.

### GET/POST /api/schedule
Manages automation scheduling with configurable intervals and repeat counts.

## Architecture Overview

### Core Components

#### USB Operations (`usb_operations.cpp`)
- Native ESP32-S3 USB host implementation
- Control transfer handling
- Device enumeration and management
- checkm8 exploit stages implementation

#### Web API (`web_api.cpp`)
- RESTful API endpoints
- JSON request/response handling
- Configuration management
- Real-time status reporting

#### Main Application (`checkm8_esp32s3.cpp`)
- FreeRTOS task management
- Hardware initialization
- State machine implementation
- WiFi and web server setup

### Task Architecture
```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   USB Host      │    │   Automation    │    │   Web Server    │
│     Task        │    │     Task        │    │     Task        │
│                 │    │                 │    │                 │
│ • Device detect │    │ • Schedule mgmt │    │ • HTTP requests │
│ • Enumeration   │    │ • Auto triggers │    │ • API endpoints │
│ • Exploit exec  │    │ • State machine │    │ • WebSocket     │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         └───────────────────────┼───────────────────────┘
                                 │
                    ┌─────────────────┐
                    │  Event Queue    │
                    │                 │
                    │ • USB events    │
                    │ • Timer events  │
                    │ • User triggers │
                    └─────────────────┘
```

## Exploit Flow

### Stage 1: Device Detection
1. Monitor USB ports for device connection
2. Enumerate device descriptors
3. Verify Apple DFU device (VID: 0x05AC, PID: 0x1227)
4. Extract serial number index

### Stage 2: Heap Feng-Shui
1. Send initial stall setup request
2. Perform 620 successful heap allocation requests
3. Send final request with specific length
4. Prepare heap layout for exploitation

### Stage 3: Global State Setting
1. Send data packets to establish global state
2. Perform zero-length packet handling
3. Execute get status requests
4. Send padding data to position heap

### Stage 4: Heap Occupation
1. Send overwrite data to corrupt heap structures
2. Deploy ARM payload for code execution
3. Trigger payload execution
4. Verify exploitation success

## Status Indicators

### LED Patterns
- **Off**: System idle, no device connected
- **Solid On**: Device detected and ready, or exploit completed successfully
- **Slow Blink**: Exploit in progress
- **Fast Blink**: Error state or exploit failed

### Web Interface Status
- **Green**: System ready, device detected
- **Yellow**: Exploit in progress
- **Red**: Error state
- **Blue**: Automation active

## Troubleshooting

### Common Issues

#### Device Not Detected
- Verify USB-C cable connection
- Ensure device is in DFU mode
- Check USB power control (GPIO21)
- Monitor serial output for enumeration errors

#### Exploit Fails
- Verify target device compatibility (A5/A5X only)
- Check USB timing and transfer completeness
- Review exploit stage progression in logs
- Ensure stable power supply

#### Web Interface Inaccessible
- Verify WiFi connection to ESP32S3-CheckM8
- Check IP address assignment (should be 192.168.4.x)
- Ensure ESP32-S3 is powered and running
- Try connecting to fallback IP: 192.168.4.1

#### Automation Not Working
- Verify automation is enabled in configuration
- Check interval settings (minimum 1000ms)
- Ensure repeat count is set appropriately
- Monitor automation task in serial logs

### Debug Mode
Enable debug mode by setting `CORE_DEBUG_LEVEL=5` in build flags:
```ini
build_flags = 
    ${env:esp32s3.build_flags}
    -DCORE_DEBUG_LEVEL=5
```

## Security Considerations

### Network Security
- Default WiFi password should be changed for production use
- Consider using WPA3 security for enhanced protection
- Web interface should be used only on trusted networks

### USB Security
- Device enumeration logs may contain sensitive information
- USB ports should be physically secured in production
- Consider adding authentication to web interface for shared deployments

## Contributing

### Development Setup
1. Fork the repository
2. Create feature branch
3. Implement changes with comprehensive testing
4. Update documentation as needed
5. Submit pull request with detailed description

### Testing
- Test on actual ESP32-S3 hardware
- Verify with multiple iOS device models
- Test automation scenarios thoroughly
- Validate web interface functionality

## License

This project is licensed under the MIT License - see the [LICENSE](../LICENSE) file for details.

## Acknowledgments

- Original checkm8 research by axi0mX
- Arduino-based implementation by DSecurity team
- ESP32-S3 USB host libraries by Espressif
- Community contributions and testing

## Version History

### v1.0.0 (Current)
- Initial ESP32-S3 implementation
- Dual USB-C support
- Web interface and automation
- FreeRTOS task architecture
- Complete self-contained operation

## Support

For support and questions:
- Create an issue on GitHub
- Check the troubleshooting section
- Review the debug logs
- Consult the ESP32-S3 documentation