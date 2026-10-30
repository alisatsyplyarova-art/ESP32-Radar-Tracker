// ESP32 Radar Tracker - Arduino IDE sketch (all firmware modules are in ./src)
// BOARD MUST BE: Tools -> Board -> esp32 -> "ESP32 Dev Module" (core 3.x (or compatible ESP32 Arduino core)), NOT Arduino Uno/AVR.
// TFT_eSPI: in User_Setup_Select.h enable  #include <User_Setups/Setup25_TTGO_T_Display.h>
// ESP32 Dual Radar LD2401 + LD2450 - LILYGO TTGO T-Display (classic ESP32)
//  Sensor 1: HLK-LD2450 on UART1   Sensor 2: LD2401-family presence radar on UART2
#include <Arduino.h>
#include <Preferences.h>
#include <math.h>
#include "Config.h"
#include "Buttons.h"
#include "RadarLD2450.h"
#include "RadarLD2401.h"
#include "RadarTargets.h"
#include "RadarDisplay.h"
#include "RadarWeb.h"

static RadarLD2450  ld2450;
static RadarLD2401  ld2401;
static RadarTargets realTargets, demoTargets;
static RadarDisplay display;
static Buttons      buttons;
static Preferences  prefs;
static RadarWeb web;
static UiSettings   settings;
static Screen       screen = SCR_RADAR;
static bool         demoMode = false;
static LD2401Data   demoPres;

static float uiFps = 0; static uint32_t fpsT0 = 0, fpsN = 0;
static uint32_t lastUi = 0, lastDemo = 0, lastLog = 0;
static LinkState lastL1 = LINK_NO_DATA, lastL2 = LINK_NO_DATA;
static int8_t lastActiveSensor = -1; // -1 none, 1 LD2401, 2 LD2450

static void saveSettings() {
  prefs.putUChar("range", settings.rangeIdx); prefs.putUChar("bright", settings.brightIdx);
  prefs.putUChar("theme", settings.theme);    prefs.putBool("trails", settings.trails);
  prefs.putBool("alien", settings.alien);     prefs.putUChar("alert", settings.alertIdx);
}

// Synthetic data, only shown while demoMode is on and always labelled DEMO.
static void demoStep(uint32_t now) {
  float t = now / 1000.0f;
  LD2450Target f[3];
  for (int i = 0; i < 3; i++) {
    if (i == 1 && ((now / 4000) % 3 == 0)) continue;           // target 2 disappears periodically
    float x = 1700.0f * sinf(t * (0.35f + 0.12f * i) + i * 2.0f);
    float y = 2800.0f + 1900.0f * sinf(t * (0.22f + 0.08f * i) + i);
    f[i].valid = true; f[i].x = (int16_t)x; f[i].y = (int16_t)y;
    f[i].speed = (int16_t)(40.0f * cosf(t * (0.22f + 0.08f * i) + i)); f[i].res = 360;
    f[i].dist = sqrtf(x * x + y * y);
  }
  demoTargets.update(f, now);
  demoPres.state = (now / 3000) % 4;
  demoPres.movDistCm = 120 + (now / 100) % 200; demoPres.statDistCm = 180; demoPres.detDistCm = demoPres.movDistCm;
  demoPres.movEnergy = 40 + (now / 50) % 50; demoPres.statEnergy = 30;
}

static void onButtons(uint32_t now) {
  if (buttons.e1 == BTN_SHORT) screen = (Screen)((screen + 1) % SCR_COUNT);
  if (buttons.e1 == BTN_LONG) {
    if (screen == SCR_SETTINGS) settings.item = (settings.item + 1) % RadarDisplay::kItems;
    else screen = (screen == SCR_DIAG) ? SCR_RADAR : SCR_DIAG;
  }
  if (buttons.e2 == BTN_SHORT) {
    if (screen == SCR_SETTINGS) {
      switch (settings.item) {
        case 0: settings.rangeIdx = (settings.rangeIdx + 1) % RadarDisplay::kRanges; break;
        case 1: settings.brightIdx = (settings.brightIdx + 1) % RadarDisplay::kBright; display.setBrightness(settings.brightIdx); break;
        case 2: settings.theme = (settings.theme + 1) % RadarDisplay::kThemes; break;
        case 3: settings.alien = !settings.alien; break;
        case 4: settings.alertIdx = (settings.alertIdx + 1) % RadarDisplay::kAlerts; break;
        case 5: settings.trails = !settings.trails; break;
      }
      saveSettings();
    } else {
      settings.rangeIdx = (settings.rangeIdx + 1) % RadarDisplay::kRanges; saveSettings();
    }
  }
  if (buttons.e2 == BTN_LONG) {
    demoMode = !demoMode; demoTargets.clear();
    Serial.println(demoMode ? F("[UI] DEMO mode ON (synthetic data)") : F("[UI] DEMO mode OFF (real data)"));
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println(F("\n=== ESP32 Dual Radar LD2401 + LD2450 | TTGO T-Display (classic ESP32) ==="));
  prefs.begin("radar", false);
  settings.rangeIdx = prefs.getUChar("range", 6) % RadarDisplay::kRanges;
  settings.brightIdx = prefs.getUChar("bright", 3) % RadarDisplay::kBright;
  settings.theme = prefs.getUChar("theme", 0) % RadarDisplay::kThemes;
  settings.trails = prefs.getBool("trails", true);
  settings.alien = prefs.getBool("alien", true);
  settings.alertIdx = prefs.getUChar("alert", 0) % RadarDisplay::kAlerts;
  if (!display.begin()) Serial.println(F("[UI] display init problem"));
  display.setBrightness(settings.brightIdx);
  buttons.begin();
  pinMode(PIN_LD2401_OUT, INPUT);
  web.begin();
  ld2450.begin(Serial1, PIN_LD2450_RX, PIN_LD2450_TX, LD2450_BAUD);
  ld2401.begin(Serial2, PIN_LD2401_RX, PIN_LD2401_TX, LD2401_BAUD);
  Serial.printf("[LD2450] UART1 RX=%d TX=%d %lu baud\n", PIN_LD2450_RX, PIN_LD2450_TX, (unsigned long)LD2450_BAUD);
  Serial.printf("[LD2401] UART2 RX=%d TX=%d %lu baud\n", PIN_LD2401_RX, PIN_LD2401_TX, (unsigned long)LD2401_BAUD);
}

void loop() {
  uint32_t now = millis();

  web.loop();
  buttons.update(now);
  onButtons(now);

  // each sensor is serviced independently: one failing never blocks the other
  ld2450.update(now);
  ld2401.update(now);
  web.sample(now, ld2401, digitalRead(PIN_LD2401_OUT));

  LD2450Target fr[3];
  if (ld2450.consumeFrame(fr)) realTargets.update(fr, now);
  if (ld2450.link(now) != LINK_OK) realTargets.clear();   // never keep targets from a dead link
  realTargets.expire(now);

  if (demoMode && now - lastDemo >= 100) { lastDemo = now; demoStep(now); }
  if (demoMode) demoTargets.expire(now);

  // serial diagnostics: link transitions + 1 s summary
  LinkState l1 = ld2450.link(now), l2 = ld2401.link(now);
  if (l1 != lastL1) { Serial.printf("[LD2450] link %s\n", linkStateStr(l1)); lastL1 = l1; }
  if (l2 != lastL2) { Serial.printf("[LD2401] link %s\n", linkStateStr(l2)); lastL2 = l2; }
  int8_t activeSensor = (l1 == LINK_OK) ? 2 : ((l2 == LINK_OK) ? 1 : 0);
  if (activeSensor != lastActiveSensor) {
    if (activeSensor == 2) Serial.println(F("[AUTO] Selected sensor: LD2450 (multi-target X/Y)"));
    else if (activeSensor == 1) Serial.println(F("[AUTO] Selected sensor: LD2401 (presence/distance)"));
    else Serial.println(F("[AUTO] No valid sensor detected yet"));
    lastActiveSensor = activeSensor;
  }
  if (now - lastLog >= 1000) {
    lastLog = now;
    Serial.printf("[LD2450] %s ok=%lu bad=%lu rej=%lu %.1ffps tgt=%u | [LD2401] %s ok=%lu bad=%lu %.1ffps st=%s\n",
      linkStateStr(l1), (unsigned long)ld2450.stats().framesOk, (unsigned long)ld2450.stats().framesBad,
      (unsigned long)ld2450.rejected(), ld2450.stats().fps, realTargets.liveCount(now),
      linkStateStr(l2), (unsigned long)ld2401.stats().framesOk, (unsigned long)ld2401.stats().framesBad,
      ld2401.stats().fps, RadarLD2401::stateStr(ld2401.data().state));
  }

  if (now - lastUi >= UI_FRAME_MS) {
    lastUi = now;
    UiContext c;
    // Automatic sensor selection: the main screen follows the sensor that has
    // a valid data link. LD2401 is 1-D presence/distance; LD2450 is X/Y tracking.
    // If both appear valid, prefer LD2450; hardware setup should connect only one.
    c.screen = screen;
    if (!demoMode && screen == SCR_RADAR && l2 == LINK_OK && l1 != LINK_OK) {
      c.screen = SCR_PRESENCE;
    }
    c.demo = demoMode; c.now = now;
    c.targets = demoMode ? &demoTargets : &realTargets;
    c.ld2450 = &ld2450; c.ld2401 = &ld2401;
    if (demoMode) { c.presValid = true; c.pres = demoPres; }
    else { c.presValid = (l2 == LINK_OK); c.pres = ld2401.data(); }
    c.uiFps = uiFps; c.set = &settings;
    display.render(c);
    fpsN++;
    if (now - fpsT0 >= 1000) { uiFps = fpsN * 1000.0f / (now - fpsT0); fpsN = 0; fpsT0 = now; }
  }
}
