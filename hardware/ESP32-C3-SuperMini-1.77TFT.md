# ESP32-C3 LD-Radar — confirmed hardware profile

## Board and display
- MCU: ESP32-C3 SuperMini.
- TFT: 1.77 inch, ST7735, 160x128 logical landscape.
- The display carrier is plugged directly onto the SuperMini; it is not wired to the MCU with separate display leads.
- The user confirmed the adapter-to-GPIO correspondence below.

## Display signal mapping

| Display signal | ESP32-C3 GPIO |
|---|---:|
| GND | GND |
| VCC | 3.3V |
| SCK | GPIO4 |
| SDA / MOSI | GPIO3 |
| RES | GPIO2 |
| RS / DC | GPIO1 |
| CS | GPIO0 |

LEDA/backlight is powered through the display carrier; no separate backlight GPIO is used in C3-0.1.0.

## Radar UART (separate wires)

| Radar module | ESP32-C3 |
|---|---|
| TX | GPIO20 (RX) |
| RX | GPIO21 (TX) |
| GND | GND |
| VCC | According to the radar module specification |

UART: 256000 baud, 8N1. Connect only one sensor at a time (LD2401 or LD2450). ESP32-C3 GPIO is 3.3V logic; do not apply 5V to GPIO.

## Hardware notes
- GPIO20/21 are routed to UART1 by the firmware. Verify on the actual SuperMini revision that they are not occupied by an external USB-UART bridge.
- GPIO0 is assigned to TFT CS as explicitly confirmed for the display carrier.
- GPIO2 is assigned to TFT reset as explicitly confirmed.
- No physical buttons, buzzer or LD2401 OUT line are wired in this build.
- ST7735 tab offset/color order and physical panel orientation still require first-power-on verification.

## Firmware
See [C3-0.1.0 firmware](../firmware/ESP32_C3_LD_Radar_0.1.0/README.md).
