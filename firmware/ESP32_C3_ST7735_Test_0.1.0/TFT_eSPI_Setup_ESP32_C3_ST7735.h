#pragma once
// ESP32-C3 SuperMini + 1.77-inch ST7735 128x160 panel.
// Landscape is selected in the sketch with setRotation(1).
#define USER_SETUP_LOADED
#define ST7735_DRIVER
#define TFT_WIDTH  128
#define TFT_HEIGHT 160
#define TFT_MOSI 3
#define TFT_SCLK 4
#define TFT_CS   0
#define TFT_DC   1
#define TFT_RST  2
#define TFT_BL  -1
#define TFT_RGB_ORDER TFT_BGR
#define SPI_FREQUENCY 10000000
#define SPI_READ_FREQUENCY 10000000
