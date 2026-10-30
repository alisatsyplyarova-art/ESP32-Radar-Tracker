#pragma once
#include <Arduino.h>
#include <FS.h>
using fs::FS;  // ESP32 Arduino Core 3.x exposes FS as fs::FS
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "RadarLD2401.h"
#include "Buttons.h"

// One measurement for the web log, whichever sensor is active.
struct WebSample {
  bool     linkOk = false;
  int16_t  cm = -1;        // distance, cm; -1 = none
  uint8_t  state = 0;      // 0 none, 1 moving, 2 static, 3 both
  int16_t  energy = -1;    // percent; -1 = sensor does not report it (LD2450)
  uint32_t frames = 0, bad = 0;
  uint8_t  sensor = 0;     // 0 none, 1 LD2401, 2 LD2450
  bool targetValid[3] = {false,false,false};
  int16_t targetX[3] = {0,0,0}, targetY[3] = {0,0,0};
};

// Snapshot of device settings, exposed as direct web controls.
struct WebUiState {
  uint8_t screen = 0;      // 0 RADAR, 1 TARGETS, 2 PRESENCE, 3 DUAL, 4 DIAG, 5 SETTINGS
  uint8_t item = 0;        // selected row on SETTINGS (0..5)
  uint8_t rangeIdx = 0, brightIdx = 0, theme = 0, alertIdx = 0;
  bool alien = true, trails = true, demo = false, buzzer = false;
  uint8_t zoneMask = 0x01; // bit0 front, bit1 left, bit2 right, bit3 rear
  uint16_t minRangeCm = 50, maxRangeCm = 500; // inclusive range filter, cm
};

class RadarWeb {
public:
  void begin();
  void loop();
  void sample(uint32_t now, const WebSample& s, int outLevel);
  void setUiState(const WebUiState& u) { ui_ = u; }
  // Direct web actions; these change the selected device function, not simulated button presses.
  struct Action { String name; int value = -1; bool valid = false; };
  bool takeAction(Action& out);
private:
  struct Row { uint32_t ms; int16_t cm; uint8_t state; uint8_t out; int16_t energy; uint8_t alarm; };
  WebServer server_{80};
  Preferences wifiPrefs_;
  Row rows_[80]{};
  uint8_t head_=0, count_=0;
  uint32_t lastSample_=0;
  uint32_t frames_=0, bad_=0;
  uint8_t sensor_=0;
  bool targetValid_[3]={false,false,false}; int16_t targetX_[3]={0,0,0}, targetY_[3]={0,0,0};
  WebUiState ui_;
  Action pendingAction_;
  String staStatus_;
  String ssid_, password_;
  void loadWifi();
  void connectSta();
  void handleRoot();
  void handleApi();
  void handleCsv();
  void handleWifi();
  void handleAction();
  const char* stateName(uint8_t state) const;
};
