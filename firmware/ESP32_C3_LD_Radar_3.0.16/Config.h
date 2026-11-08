#pragma once
#include <Arduino.h>

// ESP32-C3 SuperMini + 1.77" TFT ST7735 (160x128).
// Display carrier mapping confirmed by user:
// GND, 3V3, SCK=GPIO4, SDA/MOSI=GPIO3, RES=GPIO2, RS/DC=GPIO1, CS=GPIO0.
#define PIN_TFT_SCLK 4
#define PIN_TFT_MOSI 3
#define PIN_TFT_RST  2
#define PIN_TFT_DC   1
#define PIN_TFT_CS   0
// Backlight LEDA is powered by the display carrier; no GPIO control is wired.
#define PIN_TFT_BL   -1

// Shared UART: connect only ONE sensor at a time.
// Sensor TX -> ESP32-C3 RX GPIO20; sensor RX <- ESP32-C3 TX GPIO21.
#define PIN_RADAR_RX 20
#define PIN_RADAR_TX 21
#define LD2450_BAUD 256000UL
#define LD2401_BAUD 256000UL
#define PIN_LD2450_RX PIN_RADAR_RX
#define PIN_LD2450_TX PIN_RADAR_TX
#define PIN_LD2401_RX PIN_RADAR_RX
#define PIN_LD2401_TX PIN_RADAR_TX

// This compact C3 build has no physical buttons, buzzer or LD2401 OUT wire.
#define PIN_BTN_1 -1
#define PIN_BTN_2 -1
#define PIN_ALARM_BUZZER -1
#define PIN_LD2401_OUT -1

#define LD2450_SEND_MULTI_TARGET_CMD 1
#define LD2450_POSITIVE_SPEED_IS_AWAY 1

#define LINK_TIMEOUT_MS        1500
#define LINK_GARBAGE_AFTER_MS  2000
#define RESTART_AFTER_MS       6000

#define TARGET_HOLD_MS     300
#define TARGET_TIMEOUT_MS  1000
#define TARGET_SMOOTH      0.45f
#define TRAIL_LEN          900  // 5 min at one stored point every 333 ms
#define WEB_TRAIL_LEN      180
#define TARGET_MATCH_GATE_MM 800
#define LD2450_MAX_ABS_X   4000
#define LD2450_MAX_Y       8000

#define WIFI_AP_SSID      "LD-RADAR-C3"
#define WIFI_AP_PASSWORD  ""

#define UI_FRAME_MS        20
#define SWEEP_PERIOD_MS    2400
#define BTN_DEBOUNCE_MS    30
#define BTN_LONG_MS        700
