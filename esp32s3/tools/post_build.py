#!/usr/bin/env python3
"""
Post-build script for ESP32-S3 checkm8 project
Generates deployment information and validates build
"""

import os
import sys
import json
from datetime import datetime

def validate_build():
    """Validate the build output"""
    print("🔍 Validating ESP32-S3 checkm8 build...")
    
    # Check for firmware binary
    firmware_files = [
        ".pio/build/esp32s3/firmware.bin",
        ".pio/build/esp32s3/bootloader.bin",
        ".pio/build/esp32s3/partitions.bin"
    ]
    
    for file in firmware_files:
        if os.path.exists(file):
            size = os.path.getsize(file)
            print(f"✅ {file} ({size} bytes)")
        else:
            print(f"❌ Missing firmware file: {file}")
    
    print("✅ Build validation completed")

def generate_deployment_info():
    """Generate deployment information"""
    deployment_info = {
        "project": "checkm8-esp32s3",
        "version": "1.0.0",
        "build_time": datetime.now().isoformat(),
        "target": "esp32-s3-devkitc-1",
        "features": {
            "dual_usb_c": True,
            "automation": True,
            "web_interface": True,
            "self_contained": True
        },
        "deployment": {
            "wifi_ap": "ESP32S3-CheckM8",
            "wifi_password": "checkm8esp32",
            "web_interface": "http://192.168.4.1",
            "status_led": "GPIO2",
            "trigger_button": "GPIO0",
            "usb_power": "GPIO21"
        },
        "usage": [
            "Connect to ESP32S3-CheckM8 WiFi network",
            "Open http://192.168.4.1 in browser",
            "Connect iOS device in DFU mode to USB-C port",
            "Use web interface or GPIO trigger to start exploit"
        ]
    }
    
    with open("deployment_info.json", "w") as f:
        json.dump(deployment_info, f, indent=2)
    
    print("✅ Deployment information generated")
    
    # Print deployment summary
    print("\n📋 DEPLOYMENT SUMMARY:")
    print("=" * 50)
    print(f"Project: {deployment_info['project']}")
    print(f"Version: {deployment_info['version']}")
    print(f"Target: {deployment_info['target']}")
    print(f"WiFi AP: {deployment_info['deployment']['wifi_ap']}")
    print(f"Web Interface: {deployment_info['deployment']['web_interface']}")
    print("\n🎯 Key Features:")
    for feature, enabled in deployment_info['features'].items():
        status = "✅" if enabled else "❌"
        print(f"  {status} {feature.replace('_', ' ').title()}")

def main():
    print("🏁 ESP32-S3 checkm8 post-build script")
    validate_build()
    generate_deployment_info()
    print("✅ Post-build completed successfully")

if __name__ == "__main__":
    main()