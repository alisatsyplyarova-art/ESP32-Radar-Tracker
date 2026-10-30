#pragma once
#include <Arduino.h>
#include <FS.h>
using fs::FS;
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "RadarLD2401.h"
class RadarWeb {
public: void begin(); void loop(); void sample(uint32_t now,const RadarLD2401& radar,int outLevel);
private:
 struct Row { uint32_t ms; int16_t cm; uint8_t state,out; uint16_t energy; };
 WebServer server_{80}; Preferences wifiPrefs_; Row rows_[80]{}; uint8_t head_=0,count_=0;
 uint32_t lastSample_=0,frames_=0,bad_=0; String staStatus_,ssid_,password_;
 void loadWifi(); void connectSta(); void handleRoot(); void handleApi(); void handleCsv(); void handleWifi();
 const char* stateName(uint8_t state) const; int currentDistance(const LD2401Data& d) const;
};