#include <Arduino.h>
#include "constants.h"
#include <TinyUSB.h>

// Simple state machine for the checkm8 exploit
enum Checkm8State {
  CHECKM8_INIT_RESET,
  CHECKM8_HEAP_FENG_SHUI,
  CHECKM8_SET_GLOBAL_STATE,
  CHECKM8_HEAP_OCCUPATION,
  CHECKM8_END
};

static Checkm8State checkm8_state = CHECKM8_INIT_RESET;
static bool is_dfu = false;
static tusb_desc_device_t dev_desc;

void send_setup(uint8_t bmRequestType, uint8_t bRequest,
                uint16_t wValue, uint16_t wIndex, uint16_t wLength) {
  tusb_control_request_t setup = {
    .bmRequestType = bmRequestType,
    .bRequest = bRequest,
    .wValue = wValue,
    .wIndex = wIndex,
    .wLength = wLength,
  };
  tuh_control_xfer(0, &setup, NULL, NULL); // schedule
}

void send_data(const uint8_t *data, uint16_t len) {
  tuh_control_xfer(0, NULL, (void*)data, len);
}

void heap_feng_shui() {
  Serial.println("1. heap feng-shui");
  // TODO: implement TinyUSB host transfers
}

void set_global_state() {
  Serial.println("2. set global state");
  // TODO: implement TinyUSB host transfers
}

void heap_occupation() {
  Serial.println("3. heap occupation");
  // TODO: implement TinyUSB host transfers
}

void handle_device_mounted(uint8_t dev_addr) {
  // Query device descriptor once mounted
  tuh_descriptor_get_device_sync(dev_addr, &dev_desc);
  if (dev_desc.idVendor == 0x05ac && dev_desc.idProduct == 0x1227) {
    is_dfu = true;
    checkm8_state = CHECKM8_INIT_RESET;
  }
}

void handle_device_unmounted(uint8_t dev_addr) {
  (void)dev_addr;
  is_dfu = false;
}

void process_checkm8() {
  if (!is_dfu) return;
  switch (checkm8_state) {
    case CHECKM8_INIT_RESET:
      heap_feng_shui();
      checkm8_state = CHECKM8_SET_GLOBAL_STATE;
      break;
    case CHECKM8_SET_GLOBAL_STATE:
      set_global_state();
      checkm8_state = CHECKM8_HEAP_OCCUPATION;
      break;
    case CHECKM8_HEAP_OCCUPATION:
      heap_occupation();
      checkm8_state = CHECKM8_END;
      break;
    case CHECKM8_END:
      Serial.println("Done!");
      is_dfu = false;
      break;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 checkm8 host");
  TinyUSB_Device_Init(0);
  TinyUSBHost.begin(0); // port 0
  TinyUSBHost.begin(1); // port 1
}

void loop() {
  TinyUSBHost.task();
  process_checkm8();
}

// Callbacks required by TinyUSB host API
extern "C" void tuh_mount_cb(uint8_t dev_addr) { handle_device_mounted(dev_addr); }
extern "C" void tuh_umount_cb(uint8_t dev_addr) { handle_device_unmounted(dev_addr); }
