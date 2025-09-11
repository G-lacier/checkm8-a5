/**
 * ESP32-S3 Enhanced checkm8 Header
 * 
 * Definitions, structures, and function declarations for the ESP32-S3 checkm8 implementation
 */

#ifndef CHECKM8_ESP32S3_H
#define CHECKM8_ESP32S3_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "usb/usb_host.h"

#ifdef __cplusplus
extern "C" {
#endif

// checkm8 exploit states
typedef enum {
    CHECKM8_IDLE,
    CHECKM8_INIT_RESET,
    CHECKM8_HEAP_FENG_SHUI,
    CHECKM8_SET_GLOBAL_STATE,
    CHECKM8_HEAP_OCCUPATION,
    CHECKM8_COMPLETE,
    CHECKM8_ERROR
} CheckM8State;

// USB port states for dual USB-C support
typedef enum {
    USB_PORT_IDLE,
    USB_PORT_HOST,
    USB_PORT_DEVICE,
    USB_PORT_OTG,
    USB_PORT_ERROR
} USBPortMode;

typedef struct {
    USBPortMode mode;
    bool device_connected;
    uint8_t device_address;
    uint16_t vendor_id;
    uint16_t product_id;
} USBPortState;

// Automation configuration
typedef struct {
    bool enabled;
    uint32_t interval_ms;
    uint32_t repeat_count;
    bool auto_exploit;
} AutomationConfig;

// USB control transfer wrapper for ESP32-S3
typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_setup_packet_t;

// Function declarations for core checkm8 operations
esp_err_t send_usb_control_transfer(uint8_t device_addr, const usb_setup_packet_t *setup, 
                                   const uint8_t *data, uint16_t data_len, uint32_t timeout_ms);
esp_err_t perform_heap_feng_shui(void);
esp_err_t perform_set_global_state(void);
esp_err_t perform_heap_occupation(void);

// Device management functions
void handle_device_connection(uint8_t device_addr);
void handle_device_disconnection(uint8_t device_addr);
void execute_checkm8_state(void);

// Web API handlers
void handle_api_status(void);
void handle_api_exploit(void);
void handle_api_get_config(void);
void handle_api_set_config(void);
void handle_api_reset(void);

// Utility functions
String generate_dashboard_html(void);
esp_err_t configure_usb_port(uint8_t port_num, USBPortMode mode);
void update_status_led(CheckM8State state);

// Hardware abstraction
#define ESP32S3_USB_HOST_PORT 0
#define ESP32S3_USB_DEVICE_PORT 1

// Timing constants
#define CHECKM8_TIMEOUT_MS 5000
#define USB_TRANSFER_TIMEOUT_MS 1000
#define AUTOMATION_MIN_INTERVAL_MS 1000

// Status LED patterns
#define LED_PATTERN_IDLE 0
#define LED_PATTERN_WORKING 1
#define LED_PATTERN_SUCCESS 2
#define LED_PATTERN_ERROR 3

#ifdef __cplusplus
}
#endif

#endif // CHECKM8_ESP32S3_H