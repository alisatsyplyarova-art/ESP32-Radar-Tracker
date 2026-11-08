# ESP32-C3 ST7735 display test 0.1.0

This is a standalone display test. It does not start Wi-Fi, access Preferences, or communicate with LD2401/LD2450.

## Wiring

| Display | ESP32-C3 SuperMini |
|---|---|
| GND | GND |
| VCC | 3.3V |
| SCK | GPIO4 |
| SDA / MOSI | GPIO3 |
| RES | GPIO2 |
| RS / DC | GPIO1 |
| CS | GPIO0 |

Backlight LEDA is supplied by the display adapter.

## Arduino IDE

- Board: ESP32C3 Dev Module
- ESP32 Arduino core: 3.3.12
- Library: TFT_eSPI 2.5.43
- Open `ESP32_C3_ST7735_Test_0.1.0.ino`.

The CI workflow installs TFT_eSPI and selects the bundled setup header. For local Arduino IDE compilation, copy `TFT_eSPI_Setup_ESP32_C3_ST7735.h` to `TFT_eSPI/User_Setups/User_Setup_C3_ST7735.h`, then set `User_Setup_Select.h` to:

```cpp
#include <User_Setups/User_Setup_C3_ST7735.h>
```

Open Serial Monitor at 115200 baud. The display should show the title and then cycle through RED, GREEN, BLUE, WHITE and BLACK every 2.5 seconds.

If the panel lights but remains white, report the complete serial log and whether any color/text appears. Do not reconnect the radar sensor during this test.
