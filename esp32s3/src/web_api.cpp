/**
 * Web API Handlers for ESP32-S3 checkm8
 * 
 * REST API endpoints for remote control and automation of the checkm8 exploit
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include "esp_log.h"
#include "../include/checkm8_esp32s3.h"

static const char* TAG = "checkm8_api";

// External globals
extern CheckM8State g_checkm8_state;
extern bool g_dfu_device_found;
extern uint8_t g_current_device_addr;
extern AutomationConfig g_automation_config;
extern WebServer g_web_server;
extern USBPortState g_usb_ports[2];

/**
 * Handle API status request - returns current system state
 */
void handle_api_status(void) {
    ESP_LOGD(TAG, "API status request received");
    
    DynamicJsonDocument doc(1024);
    
    // System state
    const char* state_names[] = {
        "idle", "init_reset", "heap_feng_shui", 
        "set_global_state", "heap_occupation", "complete", "error"
    };
    
    doc["state"] = state_names[g_checkm8_state];
    doc["dfu_found"] = g_dfu_device_found;
    doc["device_address"] = g_current_device_addr;
    
    // USB port status
    JsonArray ports = doc.createNestedArray("usb_ports");
    for (int i = 0; i < 2; i++) {
        JsonObject port = ports.createNestedObject();
        port["port"] = i;
        port["mode"] = g_usb_ports[i].mode;
        port["connected"] = g_usb_ports[i].device_connected;
        port["vid"] = g_usb_ports[i].vendor_id;
        port["pid"] = g_usb_ports[i].product_id;
    }
    
    // Automation status
    JsonObject automation = doc.createNestedObject("automation");
    automation["enabled"] = g_automation_config.enabled;
    automation["interval_ms"] = g_automation_config.interval_ms;
    automation["repeat_count"] = g_automation_config.repeat_count;
    automation["auto_exploit"] = g_automation_config.auto_exploit;
    
    // System info
    JsonObject system = doc.createNestedObject("system");
    system["uptime_ms"] = millis();
    system["free_heap"] = ESP.getFreeHeap();
    system["chip_model"] = ESP.getChipModel();
    system["cpu_freq_mhz"] = ESP.getCpuFreqMHz();
    
    String response;
    serializeJson(doc, response);
    
    g_web_server.send(200, "application/json", response);
    ESP_LOGD(TAG, "Status response sent: %s", response.c_str());
}

/**
 * Handle API exploit trigger request
 */
void handle_api_exploit(void) {
    ESP_LOGI(TAG, "API exploit request received");
    
    DynamicJsonDocument doc(512);
    
    if (!g_dfu_device_found) {
        doc["success"] = false;
        doc["message"] = "No DFU device connected";
        doc["error_code"] = "NO_DEVICE";
    } else if (g_checkm8_state != CHECKM8_IDLE && g_checkm8_state != CHECKM8_COMPLETE) {
        doc["success"] = false;
        doc["message"] = "Exploit already in progress";
        doc["error_code"] = "ALREADY_RUNNING";
    } else {
        // Start the exploit
        g_checkm8_state = CHECKM8_INIT_RESET;
        doc["success"] = true;
        doc["message"] = "Exploit started successfully";
        doc["device_address"] = g_current_device_addr;
        
        ESP_LOGI(TAG, "checkm8 exploit started via API");
    }
    
    String response;
    serializeJson(doc, response);
    
    int status_code = doc["success"] ? 200 : 400;
    g_web_server.send(status_code, "application/json", response);
}

/**
 * Handle API configuration retrieval
 */
void handle_api_get_config(void) {
    ESP_LOGD(TAG, "API get config request received");
    
    DynamicJsonDocument doc(1024);
    
    // Automation configuration
    JsonObject automation = doc.createNestedObject("automation");
    automation["enabled"] = g_automation_config.enabled;
    automation["interval_ms"] = g_automation_config.interval_ms;
    automation["repeat_count"] = g_automation_config.repeat_count;
    automation["auto_exploit"] = g_automation_config.auto_exploit;
    
    // USB port configuration
    JsonArray usb_config = doc.createNestedArray("usb_ports");
    for (int i = 0; i < 2; i++) {
        JsonObject port = usb_config.createNestedObject();
        port["port"] = i;
        port["mode"] = g_usb_ports[i].mode;
        
        const char* mode_names[] = {"idle", "host", "device", "otg", "error"};
        port["mode_name"] = mode_names[g_usb_ports[i].mode];
    }
    
    // Hardware configuration
    JsonObject hardware = doc.createNestedObject("hardware");
    hardware["status_led_pin"] = 2;
    hardware["trigger_pin"] = 0;
    hardware["usb_power_pin"] = 21;
    
    // Network configuration
    JsonObject network = doc.createNestedObject("network");
    network["ap_ssid"] = "ESP32S3-CheckM8";
    network["ip_address"] = WiFi.softAPIP().toString();
    network["connected_clients"] = WiFi.softAPgetStationNum();
    
    String response;
    serializeJson(doc, response);
    
    g_web_server.send(200, "application/json", response);
}

/**
 * Handle API configuration update
 */
void handle_api_set_config(void) {
    ESP_LOGI(TAG, "API set config request received");
    
    if (!g_web_server.hasArg("plain")) {
        g_web_server.send(400, "application/json", 
                         "{\"success\":false,\"message\":\"No JSON body provided\"}");
        return;
    }
    
    String body = g_web_server.arg("plain");
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        ESP_LOGE(TAG, "JSON parsing error: %s", error.c_str());
        g_web_server.send(400, "application/json", 
                         "{\"success\":false,\"message\":\"Invalid JSON\"}");
        return;
    }
    
    DynamicJsonDocument response_doc(512);
    bool config_changed = false;
    
    // Update automation configuration
    if (doc.containsKey("automation")) {
        JsonObject automation = doc["automation"];
        
        if (automation.containsKey("enabled")) {
            g_automation_config.enabled = automation["enabled"];
            config_changed = true;
        }
        
        if (automation.containsKey("interval_ms")) {
            uint32_t interval = automation["interval_ms"];
            if (interval >= AUTOMATION_MIN_INTERVAL_MS) {
                g_automation_config.interval_ms = interval;
                config_changed = true;
            } else {
                response_doc["warning"] = "Interval too small, minimum is " + 
                                        String(AUTOMATION_MIN_INTERVAL_MS) + "ms";
            }
        }
        
        if (automation.containsKey("repeat_count")) {
            g_automation_config.repeat_count = automation["repeat_count"];
            config_changed = true;
        }
        
        if (automation.containsKey("auto_exploit")) {
            g_automation_config.auto_exploit = automation["auto_exploit"];
            config_changed = true;
        }
    }
    
    // Update USB port configuration
    if (doc.containsKey("usb_ports")) {
        JsonArray ports = doc["usb_ports"];
        for (JsonObject port : ports) {
            int port_num = port["port"];
            if (port_num >= 0 && port_num < 2) {
                if (port.containsKey("mode")) {
                    USBPortMode new_mode = (USBPortMode)port["mode"];
                    if (configure_usb_port(port_num, new_mode) == ESP_OK) {
                        g_usb_ports[port_num].mode = new_mode;
                        config_changed = true;
                        ESP_LOGI(TAG, "USB port %d mode changed to %d", port_num, new_mode);
                    }
                }
            }
        }
    }
    
    response_doc["success"] = true;
    response_doc["message"] = config_changed ? "Configuration updated successfully" : 
                                              "No configuration changes made";
    response_doc["config_changed"] = config_changed;
    
    String response;
    serializeJson(response_doc, response);
    
    g_web_server.send(200, "application/json", response);
    
    if (config_changed) {
        ESP_LOGI(TAG, "Configuration updated successfully");
    }
}

/**
 * Handle API reset request
 */
void handle_api_reset(void) {
    ESP_LOGI(TAG, "API reset request received");
    
    // Reset checkm8 state
    g_checkm8_state = CHECKM8_IDLE;
    
    // Reset automation
    g_automation_config.enabled = false;
    g_automation_config.repeat_count = 0;
    
    // Reset USB port states (but keep current connections)
    for (int i = 0; i < 2; i++) {
        if (!g_usb_ports[i].device_connected) {
            g_usb_ports[i].mode = USB_PORT_IDLE;
        }
    }
    
    DynamicJsonDocument doc(256);
    doc["success"] = true;
    doc["message"] = "System reset successfully";
    doc["new_state"] = "idle";
    
    String response;
    serializeJson(doc, response);
    
    g_web_server.send(200, "application/json", response);
    
    ESP_LOGI(TAG, "System reset completed");
}

/**
 * Handle API device management requests
 */
void handle_api_devices(void) {
    ESP_LOGD(TAG, "API devices request received");
    
    DynamicJsonDocument doc(1024);
    JsonArray devices = doc.createNestedArray("devices");
    
    // Add connected devices
    for (int i = 0; i < 2; i++) {
        if (g_usb_ports[i].device_connected) {
            JsonObject device = devices.createNestedObject();
            device["port"] = i;
            device["address"] = g_usb_ports[i].device_address;
            device["vid"] = g_usb_ports[i].vendor_id;
            device["pid"] = g_usb_ports[i].product_id;
            device["is_dfu"] = (g_usb_ports[i].vendor_id == 0x05AC && 
                               g_usb_ports[i].product_id == 0x1227);
        }
    }
    
    doc["device_count"] = devices.size();
    doc["dfu_device_found"] = g_dfu_device_found;
    doc["current_device_address"] = g_current_device_addr;
    
    String response;
    serializeJson(doc, response);
    
    g_web_server.send(200, "application/json", response);
}

/**
 * Handle API automation schedule requests
 */
void handle_api_schedule(void) {
    ESP_LOGI(TAG, "API schedule request received");
    
    if (g_web_server.method() == HTTP_GET) {
        // Get current schedule
        DynamicJsonDocument doc(512);
        doc["automation_enabled"] = g_automation_config.enabled;
        doc["next_run_ms"] = g_automation_config.enabled ? 
                            g_automation_config.interval_ms : 0;
        doc["remaining_runs"] = g_automation_config.repeat_count;
        
        String response;
        serializeJson(doc, response);
        g_web_server.send(200, "application/json", response);
        
    } else if (g_web_server.method() == HTTP_POST) {
        // Schedule new automation
        if (!g_web_server.hasArg("plain")) {
            g_web_server.send(400, "application/json", 
                             "{\"success\":false,\"message\":\"No JSON body\"}");
            return;
        }
        
        String body = g_web_server.arg("plain");
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            g_web_server.send(400, "application/json", 
                             "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        
        // Parse schedule parameters
        uint32_t delay_ms = doc.containsKey("delay_ms") ? doc["delay_ms"] : 0;
        uint32_t interval_ms = doc.containsKey("interval_ms") ? 
                              doc["interval_ms"] : 30000;
        uint32_t repeat_count = doc.containsKey("repeat_count") ? 
                               doc["repeat_count"] : 1;
        bool auto_exploit = doc.containsKey("auto_exploit") ? 
                           doc["auto_exploit"] : true;
        
        // Validate parameters
        if (interval_ms < AUTOMATION_MIN_INTERVAL_MS) {
            g_web_server.send(400, "application/json", 
                             "{\"success\":false,\"message\":\"Interval too small\"}");
            return;
        }
        
        // Schedule automation
        g_automation_config.interval_ms = interval_ms;
        g_automation_config.repeat_count = repeat_count;
        g_automation_config.auto_exploit = auto_exploit;
        g_automation_config.enabled = true;
        
        DynamicJsonDocument response_doc(256);
        response_doc["success"] = true;
        response_doc["message"] = "Automation scheduled successfully";
        response_doc["interval_ms"] = interval_ms;
        response_doc["repeat_count"] = repeat_count;
        
        String response;
        serializeJson(response_doc, response);
        g_web_server.send(200, "application/json", response);
        
        ESP_LOGI(TAG, "Automation scheduled: interval=%dms, repeat=%d", 
                 (int)interval_ms, (int)repeat_count);
    }
}