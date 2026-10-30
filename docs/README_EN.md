# ESP32 Radar Tracker

Firmware for the **LILYGO TTGO T-Display (classic ESP32, ST7789 240×135)** with support for one HLK-LD2401 or HLK-LD2450 sensor.

## Features

- Radar-style display modes.
- Support for LD2401 and LD2450 (connect one sensor at a time).
- Wi-Fi access point `LD-RADAR`; web interface at `http://192.168.4.1/`.
- Sensor status, distance/signal data, recent-history graph and CSV export.
- Wi-Fi configuration through the web interface.

## Quick start

1. Open `firmware/ESP32_Radar_Tracker` in Arduino IDE.
2. Install Espressif's ESP32 board package and the **TFT_eSPI** library.
3. Configure TFT_eSPI using `TFT_eSPI_Setup25_TTGO_T_Display.h`.
4. Select the classic ESP32 board and serial port, then build and upload.
5. Connect to `LD-RADAR` and open `http://192.168.4.1/`.

Check the exact sensor model's power requirements and pinout before wiring. Do not connect LD2401 and LD2450 at the same time.

See [Russian build instructions](BUILD_RU.md), [hardware guide](HARDWARE_RU.md), and [web interface guide](WEB_INTERFACE_RU.md).
