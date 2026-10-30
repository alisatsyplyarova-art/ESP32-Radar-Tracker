#include "Buttons.h"
void Button::begin(uint8_t pin, bool usePullup) {
  pin_ = pin; pinMode(pin_, usePullup ? INPUT_PULLUP : INPUT);
}
BtnEvent Button::update(uint32_t now) {
  BtnEvent ev = BTN_NONE; bool raw = (digitalRead(pin_) == LOW);
  if (raw != lastRaw_) { lastRaw_ = raw; lastChange_ = now; }
  if ((now - lastChange_) >= BTN_DEBOUNCE_MS && raw != stable_) {
    stable_ = raw;
    if (stable_) { pressedAt_ = now; longFired_ = false; }
    else if (!longFired_) ev = BTN_SHORT;
  }
  if (stable_ && !longFired_ && (now - pressedAt_) >= BTN_LONG_MS) { longFired_ = true; ev = BTN_LONG; }
  return ev;
}
void Buttons::begin() { b1_.begin(PIN_BTN_1, true); b2_.begin(PIN_BTN_2, false); }
