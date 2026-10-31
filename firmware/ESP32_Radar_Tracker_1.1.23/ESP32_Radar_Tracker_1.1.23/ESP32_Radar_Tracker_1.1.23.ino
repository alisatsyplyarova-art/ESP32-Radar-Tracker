// ESP32 Radar Tracker 1.1.23 - Arduino IDE sketch (modules are in the sketch folder)
// BOARD MUST BE: Tools -> Board -> esp32 -> "ESP32 Dev Module" (core 3.x (or compatible ESP32 Arduino core)), NOT Arduino Uno/AVR.
// TFT_eSPI: in User_Setup_Select.h enable  #include <User_Setups/TFT_eSPI_Setup25_TTGO_T_Display.h>
// ESP32 Dual Radar LD2401 + LD2450 - LILYGO TTGO T-Display (classic ESP32)
//  Both sensors share UART1 (GPIO27 RX / GPIO26 TX, 256000 8N1); connect ONE at a time.
#include <Arduino.h>
#include <Preferences.h>
#include <math.h>
#include <esp_system.h>
#include "Config.h"
#include "Version.h"
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
static bool         buzzerEnabled = false;
static bool         alarmActive = false;
static uint8_t      zoneMask = 0x01; // front enabled by default
static uint16_t     minRangeCm = 50, maxRangeCm = 500; // valid target distance, inclusive
static LD2401Data   demoPres;

static float uiFps = 0; static uint32_t fpsT0 = 0, fpsN = 0;
static uint32_t lastUi = 0, lastDemo = 0, lastLog = 0;
static LinkState lastL1 = LINK_NO_DATA, lastL2 = LINK_NO_DATA;
static int8_t lastActiveSensor = -1; // -1 none, 1 LD2401, 2 LD2450

// Re-open the shared UART when NEITHER parser has seen a valid frame for RESTART_AFTER_MS.
// (The drivers no longer do this themselves in shared mode, see RadarLD2450::tickShared.)
static void restartSharedUartIfStuck(uint32_t now) {
  if (!ld2450.stats().needRestart(now) || !ld2401.stats().needRestart(now)) return;
  Serial1.end();
  Serial1.setRxBufferSize(2048);
  Serial1.begin(LD2450_BAUD, SERIAL_8N1, PIN_RADAR_RX, PIN_RADAR_TX);
  ld2450.sharedPortRestarted(now);
  ld2401.sharedPortRestarted(now);
  Serial.println(F("[AUTO] no valid frames from any sensor -> shared UART restarted"));
}

static bool inDistanceRange(float distanceMm) {
  return distanceMm >= (float)minRangeCm * 10.0f && distanceMm <= (float)maxRangeCm * 10.0f;
}

// Web log source: the sensor that currently has a valid link (LD2450 preferred, as on the TFT).
static WebSample makeWebSample(uint32_t now, LinkState l1, LinkState l2) {
  WebSample w;
  if (l1 == LINK_OK) {
    w.sensor = 2; w.linkOk = true; w.frames = ld2450.stats().framesOk; w.bad = ld2450.stats().framesBad;
    for (uint8_t i=0;i<3;i++) { const TrackedTarget& t=realTargets.get(i); if (realTargets.stateOf(i, now)==TS_LIVE && inDistanceRange(realTargets.get(i).rawDist)) { w.targetValid[i]=true; w.targetX[i]=t.rawX; w.targetY[i]=t.rawY; } }
    int8_t n = realTargets.nearest(now);
    if (n >= 0) {
      const TrackedTarget& t = realTargets.get(n);
      int cm = (int)(t.rawDist / 10.0f + 0.5f);
      w.cm = (cm > 0 && cm <= 600) ? cm : -1;
      w.state = (t.speed != 0) ? 1 : 2;               // moving if the sensor reports non-zero speed
    }
  } else if (l2 == LINK_OK) {
    const LD2401Data& d = ld2401.data();
    w.sensor = 1; w.linkOk = true; w.frames = ld2401.stats().framesOk; w.bad = ld2401.stats().framesBad;
    uint16_t cm = 0;
    if (d.state == 1) cm = d.movDistCm; else if (d.state == 2) cm = d.statDistCm;
    else if (d.state == 3) cm = d.movDistCm ? d.movDistCm : d.statDistCm;
    w.cm = (cm > 0 && cm <= 600 && cm >= minRangeCm && cm <= maxRangeCm) ? (int16_t)cm : -1;
    w.state = d.state; w.energy = (d.state == 2) ? d.statEnergy : d.movEnergy;
  } else {
    w.frames = ld2450.stats().framesOk + ld2401.stats().framesOk;
    w.bad = ld2450.stats().framesBad + ld2401.stats().framesBad;
  }
  return w;
}

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
  Serial.println(F("\n=== ESP32 Radar Tracker 1.1.23 | shared UART LD2401/LD2450 | TTGO T-Display ==="));
  Serial.printf("[BOOT] reset reason %d, free heap %u, largest block %u\n",
                (int)esp_reset_reason(), (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());
  prefs.begin("radar", false);
  buzzerEnabled = prefs.getBool("buzzer", false);
  zoneMask = prefs.getUChar("zones", 0x01) & 0x0F;
  minRangeCm = prefs.getUShort("mincm", 50); maxRangeCm = prefs.getUShort("maxcm", 500);
  if (minRangeCm < 10 || minRangeCm >= maxRangeCm || maxRangeCm > 2000) { minRangeCm = 50; maxRangeCm = 500; }
  pinMode(PIN_ALARM_BUZZER, OUTPUT);
  digitalWrite(PIN_ALARM_BUZZER, LOW);
  settings.rangeIdx = prefs.getUChar("range", 6) % RadarDisplay::kRanges;
  settings.brightIdx = prefs.getUChar("bright", 3) % RadarDisplay::kBright;
  settings.theme = prefs.getUChar("theme", 0) % RadarDisplay::kThemes;
  settings.trails = prefs.getBool("trails", true);
  settings.alien = prefs.getBool("alien", true);
  settings.alertIdx = prefs.getUChar("alert", 0) % RadarDisplay::kAlerts;
  if (!display.begin()) Serial.println(F("[UI] display init problem: UI will stay blank, see message on the panel"));
  Serial.printf("[BOOT] saved brightness %u/5\n", (unsigned)(settings.brightIdx + 1));
  display.setBrightness(settings.brightIdx);
  buttons.begin();
  web.setScreenBuffer(display.framebuffer(), RadarDisplay::kWidth, RadarDisplay::kHeight);
  web.begin();
  pinMode(PIN_LD2401_OUT, INPUT);
  Serial1.setRxBufferSize(2048);
  Serial1.begin(LD2450_BAUD, SERIAL_8N1, PIN_RADAR_RX, PIN_RADAR_TX);
  ld2450.beginShared(Serial1, LD2450_BAUD);
  ld2401.beginShared(Serial1, LD2401_BAUD);
  Serial.printf("[AUTO] Shared UART RX=%d TX=%d %lu baud; connect ONE sensor only\n",
                PIN_RADAR_RX, PIN_RADAR_TX, (unsigned long)LD2450_BAUD);
}

void loop() {
  uint32_t now = millis();

  web.loop();
  buttons.update(now);
  onButtons(now);
  RadarWeb::Action webAction;
  if (web.takeAction(webAction)) {
    const int v = webAction.value;
    if (webAction.name == "screen") screen = (Screen)v;
    else if (webAction.name == "range") settings.rangeIdx = (uint8_t)v;
    else if (webAction.name == "bright") { settings.brightIdx = (uint8_t)v; display.setBrightness(settings.brightIdx); }
    else if (webAction.name == "theme") settings.theme = (uint8_t)v;
    else if (webAction.name == "style") settings.alien = (v != 0);
    else if (webAction.name == "alert") settings.alertIdx = (uint8_t)v;
    else if (webAction.name == "buzzer") { buzzerEnabled = (v != 0); prefs.putBool("buzzer", buzzerEnabled); if (!buzzerEnabled) digitalWrite(PIN_ALARM_BUZZER, LOW); }
    else if (webAction.name == "zones") { zoneMask = (uint8_t)v & 0x0F; prefs.putUChar("zones", zoneMask); }
    else if (webAction.name == "minrange") { if (v < maxRangeCm) { minRangeCm = (uint16_t)v; prefs.putUShort("mincm", minRangeCm); realTargets.clear(); } }
    else if (webAction.name == "maxrange") { if (v > minRangeCm) { maxRangeCm = (uint16_t)v; prefs.putUShort("maxcm", maxRangeCm); realTargets.clear(); } }
    else if (webAction.name == "trails") settings.trails = (v != 0);
    else if (webAction.name == "demo" && demoMode != (v != 0)) { demoMode = (v != 0); demoTargets.clear(); Serial.println(demoMode ? F("[UI] DEMO mode ON (synthetic data)") : F("[UI] DEMO mode OFF (real data)")); }
    saveSettings();
  }
  {
    WebUiState u;
    u.screen = (uint8_t)screen; u.item = settings.item;
    u.rangeIdx = settings.rangeIdx; u.brightIdx = settings.brightIdx; u.theme = settings.theme; u.alertIdx = settings.alertIdx;
    u.alien = settings.alien; u.trails = settings.trails; u.demo = demoMode; u.buzzer = buzzerEnabled; u.zoneMask = zoneMask; u.minRangeCm = minRangeCm; u.maxRangeCm = maxRangeCm;
    web.setUiState(u);
  }

  // One physical UART feeds both non-blocking parsers.
  int uartBytes = 0;
  while (Serial1.available() && uartBytes < 1024) {
    const uint8_t byteIn = (uint8_t)Serial1.read();
    ld2450.feedByte(byteIn, now);
    ld2401.feedByte(byteIn, now);
    ++uartBytes;
  }
  ld2450.tickShared(now);
  ld2401.tickShared(now);

  LD2450Target fr[3];
  if (ld2450.consumeFrame(fr)) { for (uint8_t i=0;i<3;i++) if (fr[i].valid && !inDistanceRange(fr[i].dist)) fr[i].valid = false; realTargets.update(fr, now); }
  if (ld2450.link(now) != LINK_OK) realTargets.clear();   // never keep targets from a dead link
  realTargets.expire(now);

  if (demoMode && now - lastDemo >= 100) { lastDemo = now; demoStep(now); }
  if (demoMode) demoTargets.expire(now);

  // serial diagnostics: link transitions + 1 s summary
  LinkState l1 = ld2450.link(now), l2 = ld2401.link(now);
  restartSharedUartIfStuck(now);
  WebSample currentWebSample = makeWebSample(now, l1, l2);
  web.sample(now, currentWebSample, digitalRead(PIN_LD2401_OUT));
  // Demo data must never activate physical alarm hardware.
  const int alertLimitCm = settings.alertIdx == 1 ? 100 : settings.alertIdx == 2 ? 200 : settings.alertIdx == 3 ? 300 : 0;
  bool zoneAlarm = false;
  if (!demoMode && alertLimitCm > 0 && l1 == LINK_OK) {
    // X<0 left, X>0 right; Y>0 front, Y<0 behind. Front is split into centre and side sectors.
    for (uint8_t i=0;i<3;i++) if (realTargets.stateOf(i, now)==TS_LIVE) {
      const TrackedTarget& t=realTargets.get(i);
      const float ax=fabsf((float)t.rawX), ay=fabsf((float)t.rawY);
      uint8_t zone = (t.rawY < 0) ? 3 : (ax <= ay * 0.50f ? 0 : (t.rawX < 0 ? 1 : 2));
      const float d=sqrtf((float)t.rawX*t.rawX+(float)t.rawY*t.rawY);
      if (inDistanceRange(d) && (zoneMask & (1u<<zone)) && d < alertLimitCm*10.0f) zoneAlarm=true;
    }
  } else if (!demoMode && l2 == LINK_OK && alertLimitCm > 0 && currentWebSample.cm >= 0) {
    // LD2401 has no X/Y coordinates: radial alarm only.
    zoneAlarm = currentWebSample.cm < alertLimitCm;
  }
  alarmActive = (!demoMode && buzzerEnabled && zoneAlarm);
  digitalWrite(PIN_ALARM_BUZZER, alarmActive ? HIGH : LOW);
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
    else { c.presValid = (l2 == LINK_OK); c.pres = ld2401.data(); uint16_t dcm = (c.pres.state == 1) ? c.pres.movDistCm : (c.pres.state == 2) ? c.pres.statDistCm : (c.pres.movDistCm ? c.pres.movDistCm : c.pres.statDistCm); if (dcm && (dcm < minRangeCm || dcm > maxRangeCm)) { c.pres.state = 0; c.pres.movDistCm = c.pres.statDistCm = c.pres.detDistCm = 0; } }
    c.uiFps = uiFps; c.set = &settings;
    display.render(c);
    fpsN++;
    if (now - fpsT0 >= 1000) { uiFps = fpsN * 1000.0f / (now - fpsT0); fpsN = 0; fpsT0 = now; }
  }
}
