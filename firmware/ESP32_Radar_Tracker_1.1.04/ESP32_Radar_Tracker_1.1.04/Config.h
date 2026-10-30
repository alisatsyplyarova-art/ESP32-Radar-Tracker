#pragma once
// ============================================================================
//  ESP32 Dual Radar LD2401 + LD2450 - pinout and settings
//  Board: LILYGO TTGO T-Display (classic ESP32, ST7789 135x240)
// ============================================================================
#include <Arduino.h>

// ---- Pins reserved by the board (do NOT reuse) ------------------------------
//  TFT : MOSI 19, SCLK 18, CS 5, DC 16, RST 23, BL 4   (set in platformio.ini)
//  Buttons : GPIO0 (BOOT), GPIO35 (input only, external pull-up on the board)
//  Battery ADC : GPIO34, ADC enable : GPIO14
//  USB-UART (Serial / flashing) : GPIO1, GPIO3
//  Flash : GPIO6..11 (never use);  strapping: 0, 2, 5, 12, 15 (avoided)
#define PIN_BTN_1 0
#define PIN_BTN_2 35

// ---- Shared sensor UART ----------------------------------------------------
// Connect ONLY ONE sensor at a time. Both LD2401 and LD2450 use 256000 baud.
// Preserve the proven LD2401 wiring from 1.1.03: sensor TX -> GPIO27 (RX),
// sensor RX <- GPIO26 (TX), OUT -> GPIO25 (optional diagnostic input).
// LD2450 uses the same UART pins when it is the only connected sensor.
#define PIN_RADAR_RX 27
#define PIN_RADAR_TX 26
#define LD2450_BAUD 256000UL
#define LD2401_BAUD 256000UL
// Legacy aliases retained for source compatibility; both map to shared UART.
#define PIN_LD2450_RX PIN_RADAR_RX
#define PIN_LD2450_TX PIN_RADAR_TX
#define PIN_LD2401_RX PIN_RADAR_RX
#define PIN_LD2401_TX PIN_RADAR_TX
// GPIO25 remains the optional LD2401 OUT diagnostic input.
#define PIN_LD2401_OUT 25

// Send "multi-target tracking" command to the LD2450 at start-up (command 0x0090).
#define LD2450_SEND_MULTI_TARGET_CMD 1
// Sign convention of LD2450 speed. VERIFY on hardware (walk toward the sensor):
// 1 -> positive value shown as "AWAY", 0 -> positive value shown as "NEAR".
#define LD2450_POSITIVE_SPEED_IS_AWAY 1

// ---- Link supervision -------------------------------------------------------
#define LINK_TIMEOUT_MS        1500   // no valid frame for this long -> LOST
#define LINK_GARBAGE_AFTER_MS  2000   // bytes arrive but no valid frame -> BAD DATA
#define RESTART_AFTER_MS       6000   // re-open UART if no valid frame for this long

// ---- Target tracking (LD2450) ----------------------------------------------
#define TARGET_HOLD_MS     300        // marker stays "live" this long after last frame
#define TARGET_TIMEOUT_MS  1000       // marker is deleted after this long without data
#define TARGET_SMOOTH      0.45f      // EMA factor for display position (1 = no smoothing)
#define TRAIL_LEN          8
#define LD2450_MAX_ABS_X   4000       // mm, plausibility limits (sensor: +-60 deg, ~6 m)
#define LD2450_MAX_Y       8000       // mm

// ---- UI ---------------------------------------------------------------------
#define UI_FRAME_MS        33
#define SWEEP_PERIOD_MS    3000
#define BTN_DEBOUNCE_MS    30
#define BTN_LONG_MS        700
