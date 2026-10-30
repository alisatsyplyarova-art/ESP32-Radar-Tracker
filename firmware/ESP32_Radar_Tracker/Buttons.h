#pragma once
#include <Arduino.h>
#include "Config.h"
enum BtnEvent : uint8_t { BTN_NONE, BTN_SHORT, BTN_LONG };
class Button {
public:
  void begin(uint8_t pin, bool usePullup);
  BtnEvent update(uint32_t now);
private:
  uint8_t pin_ = 0;
  bool lastRaw_ = false, stable_ = false, longFired_ = false;
  uint32_t lastChange_ = 0, pressedAt_ = 0;
};
class Buttons {
public:
  void begin();
  void update(uint32_t now) { e1 = b1_.update(now); e2 = b2_.update(now); }
  BtnEvent e1 = BTN_NONE, e2 = BTN_NONE;
private:
  Button b1_, b2_;
};
