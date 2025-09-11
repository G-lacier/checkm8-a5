/**
 * ESP32-S3 Enhanced checkm8 Main Sketch
 * 
 * This is the main Arduino sketch file that includes the comprehensive ESP32-S3 
 * implementation with dual USB-C support and full automation capabilities.
 * 
 * This implementation provides:
 * - Complete self-contained checkm8 exploit for A5/A5X devices
 * - Dual USB-C port support with role switching
 * - Web-based remote control interface
 * - Automated scheduling and triggering
 * - GPIO status indicators and manual triggers
 * - Native ESP32-S3 USB host (no external MCU required)
 * 
 * Hardware Requirements:
 * - ESP32-S3-WROOM-1 development board
 * - Dual USB-C connectors (USB0 for device detection, USB1 for control)
 * - Status LED on GPIO2
 * - Manual trigger button on GPIO0
 * - USB power control on GPIO21
 * 
 * Usage:
 * 1. Connect to "ESP32S3-CheckM8" WiFi network (password: "checkm8esp32")
 * 2. Open browser to http://192.168.4.1 for web interface
 * 3. Connect iOS device in DFU mode to USB-C port
 * 4. Trigger exploit manually or configure automation
 * 
 * Author: ESP32-S3 Port Implementation
 * License: MIT
 * Version: 1.0.0
 */

// Include our enhanced ESP32-S3 implementation
#include "src/checkm8_esp32s3.cpp"

/**
 * This file serves as the main Arduino sketch entry point.
 * All the actual implementation is in the modular C++ files in the src/ directory.
 * 
 * The setup() and loop() functions are defined in checkm8_esp32s3.cpp
 * 
 * Key features of this enhanced implementation:
 * 
 * 1. FULL CONTAINMENT:
 *    - No external Arduino or MCU required
 *    - Native ESP32-S3 USB host implementation
 *    - All checkm8 logic runs on-device
 * 
 * 2. DUAL USB-C SUPPORT:
 *    - USB0: Primary device detection and exploitation
 *    - USB1: Secondary functions (logging, control interface)
 *    - Role switching and enumeration handled internally
 * 
 * 3. AUTOMATION FEATURES:
 *    - FreeRTOS task-based scheduling
 *    - Web interface for remote control
 *    - GPIO triggers and status indicators
 *    - Configurable automated workflows
 * 
 * 4. SELF-CONTAINED OPERATION:
 *    - WiFi AP mode for isolated operation
 *    - No external dependencies once flashed
 *    - Internal state management and error handling
 *    - Comprehensive logging and monitoring
 * 
 * 5. ADVANCED USB HANDLING:
 *    - Native ESP-IDF USB host stack
 *    - Device enumeration and management
 *    - Control transfer implementation
 *    - USB OTG and role detection
 * 
 * Web Interface Endpoints:
 * - / : Main dashboard
 * - /api/status : System status JSON
 * - /api/exploit : Trigger exploit
 * - /api/config : Configuration management
 * - /api/reset : System reset
 * - /api/devices : Connected device list
 * - /api/schedule : Automation scheduling
 * 
 * GPIO Mapping:
 * - GPIO2: Status LED (idle=off, working=blink, success=on, error=fast blink)
 * - GPIO0: Manual trigger button (active low)
 * - GPIO21: USB power control (high=on)
 * 
 * Automation Examples:
 * 1. Auto-exploit on device detection
 * 2. Scheduled exploitation every N minutes
 * 3. GPIO trigger for manual control
 * 4. Web API for remote automation
 * 
 * For advanced usage, see the complete API documentation in the web interface.
 */

/* The actual implementation is in the included files:
 * 
 * checkm8_esp32s3.cpp - Main application logic and setup
 * usb_operations.cpp  - Native ESP32-S3 USB host implementation
 * web_api.cpp         - REST API handlers for web interface
 * 
 * This modular approach allows for easy maintenance and extension
 * while keeping the Arduino sketch file clean and focused.
 */