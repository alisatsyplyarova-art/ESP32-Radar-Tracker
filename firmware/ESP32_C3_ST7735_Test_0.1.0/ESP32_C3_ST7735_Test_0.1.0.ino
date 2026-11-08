/*
  ESP32-C3 SuperMini + ST7735 1.77" display-only test
  Wiring:
    SCK=GPIO4, MOSI/SDA=GPIO3, RST=GPIO2, DC/RS=GPIO1, CS=GPIO0
    VCC=3V3, GND=GND
  No radar sensor, Wi-Fi, Preferences or other peripherals are used.
*/
#include <Arduino.h>
#include <TFT_eSPI.h>

static TFT_eSPI tft;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println(F("[TEST] ESP32-C3 ST7735 display test 0.1.0"));
  Serial.println(F("[TEST] GPIO SCK=4 MOSI=3 RST=2 DC=1 CS=0"));
  Serial.println(F("[TEST] Starting TFT_eSPI init..."));

  tft.init();
  Serial.println(F("[TEST] tft.init() returned"));

  tft.setRotation(1);
  Serial.printf("[TEST] Display dimensions: %d x %d\n", tft.width(), tft.height());

  tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, tft.width(), tft.height(), TFT_WHITE);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("LD-RADAR C3", 8, 7, 2);
  tft.drawString("ST7735 TEST", 8, 29, 2);
  tft.drawString("160 x 128", 8, 52, 2);
  tft.drawString("GPIO 4 3 2 1 0", 8, 77, 1);
  tft.drawString("Display only", 8, 96, 1);
  Serial.println(F("[TEST] Initial screen drawn"));
}

void loop() {
  static const uint16_t colors[] = {
    TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE, TFT_BLACK
  };
  static const char* names[] = {
    "RED", "GREEN", "BLUE", "WHITE", "BLACK"
  };
  static uint8_t index = 0;
  static uint32_t changedAt = 0;

  if (millis() - changedAt >= 2500) {
    changedAt = millis();
    tft.fillScreen(colors[index]);
    tft.drawRect(0, 0, tft.width(), tft.height(), (index == 4) ? TFT_WHITE : TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor((index == 4) ? TFT_WHITE : TFT_BLACK, colors[index]);
    tft.drawString(names[index], tft.width() / 2, tft.height() / 2, 4);
    Serial.printf("[TEST] Color: %s\n", names[index]);
    index = (index + 1) % 5;
  }
  delay(10);
}
