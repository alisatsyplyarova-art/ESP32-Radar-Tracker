#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "RadarTargets.h"
#include "RadarLD2450.h"
#include "RadarLD2401.h"

enum Screen : uint8_t { SCR_RADAR, SCR_TARGETS, SCR_PRESENCE, SCR_DUAL, SCR_DIAG, SCR_SETTINGS, SCR_COUNT };

struct UiSettings {
  uint8_t rangeIdx = 6;    // 0..6 -> 0.5 / 1 / 1.5 / 2 / 3 / 4 / 6 m
  uint8_t brightIdx = 3;   // 0..4
  uint8_t theme = 0;       // 0 green, 1 amber, 2 cyan
  bool    trails = true;
  bool    alien = true;    // ALIEN tracker style: dotted rings, phosphor fade of blips, scanlines
  uint8_t alertIdx = 0;    // 0 off, 1..3 -> 1 / 2 / 3 m alert zone
  uint8_t item = 0;        // selected row on SETTINGS
};

struct UiContext {
  Screen screen = SCR_RADAR;
  bool demo = false;
  uint32_t now = 0;
  const RadarTargets* targets = nullptr;   // real or demo targets (never mixed)
  const RadarLD2450* ld2450 = nullptr;
  const RadarLD2401* ld2401 = nullptr;
  bool presValid = false;                  // true only if data is real+link OK, or demo
  LD2401Data pres;
  float uiFps = 0;
  const UiSettings* set = nullptr;
};

class RadarDisplay {
public:
  bool begin();
  void setBrightness(uint8_t idx);
  void render(const UiContext& c);
  const uint16_t* framebuffer() { return spriteReady_ ? (const uint16_t*)spr_.getPointer() : nullptr; }
  static constexpr uint16_t kWidth = 240, kHeight = 135;
  static uint16_t rangeMm(uint8_t idx);
  RadarDisplay() : spr_(&tft_) {}
  static const uint8_t kRanges = 7, kBright = 5, kThemes = 3, kAlerts = 4, kItems = 6;

private:
  struct Theme { uint16_t accent, dim, grid; const char* name; };
  TFT_eSPI tft_;
  TFT_eSprite spr_;
  bool spriteReady_ = false;
  bool backlightPwmReady_ = false;
  Theme th_[3];
  uint16_t cBg_, cText_, cWarn_, cBad_, cGood_, cGray_, cSlot_[3];

  uint16_t scale(uint16_t c, uint8_t f) const;
  void txt(int x, int y, uint8_t font, uint16_t col, const char* fmt, ...);
  void txtC(int x, int y, uint8_t font, uint16_t col, const char* fmt, ...);
  void header(const UiContext& c, const char* title);
  void footer(const UiContext& c);
  void field(const UiContext& c, int ox, int oy, int R);
  void bar(int x, int y, int w, int h, int val, int maxv, uint16_t col);
  uint16_t linkCol(LinkState s) const;
  bool alertActive(const UiContext& c) const;

  void scrRadar(const UiContext& c);
  void scrTargets(const UiContext& c);
  void scrPresence(const UiContext& c);
  void scrDual(const UiContext& c);
  void scrDiag(const UiContext& c);
  void scrSettings(const UiContext& c);
};
