#!/usr/bin/env python3
"""
Pre-build script for ESP32-S3 checkm8 project
Validates configuration and prepares build environment
"""

import os
import sys
import json
from datetime import datetime

def validate_configuration():
    """Validate build configuration"""
    print("🔧 Validating ESP32-S3 checkm8 configuration...")
    
    # Check required files
    required_files = [
        "include/checkm8_esp32s3.h",
        "src/checkm8_esp32s3.cpp",
        "src/usb_operations.cpp",
        "src/web_api.cpp",
        "../constants.h"
    ]
    
    for file in required_files:
        if not os.path.exists(file):
            print(f"❌ Missing required file: {file}")
            sys.exit(1)
    
    print("✅ All required files present")

def generate_build_info():
    """Generate build information header"""
    build_info = {
        "build_time": datetime.now().isoformat(),
        "version": "1.0.0",
        "features": [
            "dual_usb_c",
            "automation",
            "web_interface",
            "self_contained"
        ],
        "target": "esp32-s3"
    }
    
    header_content = f"""
// Auto-generated build information
#ifndef BUILD_INFO_H
#define BUILD_INFO_H

#define BUILD_TIME "{build_info['build_time']}"
#define VERSION "{build_info['version']}"
#define TARGET "{build_info['target']}"

#define FEATURE_DUAL_USB_C 1
#define FEATURE_AUTOMATION 1
#define FEATURE_WEB_INTERFACE 1
#define FEATURE_SELF_CONTAINED 1

#endif // BUILD_INFO_H
"""
    
    with open("include/build_info.h", "w") as f:
        f.write(header_content)
    
    print("✅ Build information generated")

def main():
    print("🚀 ESP32-S3 checkm8 pre-build script")
    validate_configuration()
    generate_build_info()
    print("✅ Pre-build completed successfully")

if __name__ == "__main__":
    main()