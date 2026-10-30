# ESP32 Radar Tracker 1.1.20

Firmware for the **LILYGO TTGO T-Display (classic ESP32, ST7789 240×135)**. One sensor is connected at a time: HLK-LD2401 or HLK-LD2450.

## Features

- TFT screens: RADAR, TARGETS, PRESENCE, DUAL, DIAG and SETTINGS.
- ALIEN/CLASSIC target styles, three display themes, range and brightness settings.
- Shared UART1 for either sensor: GPIO27 RX, GPIO26 TX, 256000 baud, 8N1. Do not connect both sensors simultaneously.
- LD2450 provides X/Y target coordinates; LD2401 provides presence/motion and distance data, not X/Y coordinates.
- Wi-Fi AP `LD-RADAR`, web UI at `http://192.168.4.1/` (open by default).
- Web status, 60-second distance graph, up to 80 log records and CSV export.
- Live 240×135 TFT framebuffer preview, responsive up to 700 px and about 5 fps.
- In 1.1.20, browser preview colors are adapted to the web UI's dark/cyan palette. The physical TFT colors and selected display theme are unchanged.
- Distance filter, LD2450 alarm zones and optional buzzer output via transistor driver.

## Build

1. Open `firmware/ESP32_Radar_Tracker_1.1.20/ESP32_Radar_Tracker_1.1.20/` in Arduino IDE.
2. Install Espressif ESP32 board support and Bodmer's TFT_eSPI library.
3. Select the provided `TFT_eSPI_Setup25_TTGO_T_Display.h` in TFT_eSPI's `User_Setup_Select.h`.
4. Select **ESP32 Dev Module**, choose the serial port and run Verify/Compile.
5. Upload only after a successful build. Serial Monitor: 115200 baud.
6. Connect to `LD-RADAR` and browse to `http://192.168.4.1/`.

See [Russian build guide](BUILD_RU.md), [hardware wiring](HARDWARE_RU.md), [web UI guide](WEB_INTERFACE_RU.md), and the [1.1.20 release notes](../firmware/ESP32_Radar_Tracker_1.1.20/ESP32_Radar_Tracker_1.1.20/CHANGELOG_RU.md).

## Verification status

The 1.1.20 source files are published in GitHub. This does not imply that this revision has been compiled in Arduino IDE or tested on physical hardware.
