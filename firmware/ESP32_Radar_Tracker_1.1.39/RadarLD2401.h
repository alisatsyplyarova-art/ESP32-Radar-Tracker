#pragma once
#include <Arduino.h>
#include "LinkMonitor.h"

// HLK-LD2401 presence/motion radar driver.
// Frame: F4 F3 F2 F1 | payload length (LE) | payload | F8 F7 F6 F5.
// Basic payload: 02 AA state moveDist(LE cm) moveEnergy statDist(LE cm)
//                statEnergy detectDist(LE cm) 55 00.
// This sensor reports presence and distance only; it does NOT provide X/Y coordinates.
struct LD2401Data {
  uint8_t state = 0;
  uint16_t movDistCm = 0;
  uint8_t movEnergy = 0;
  uint16_t statDistCm = 0;
  uint8_t statEnergy = 0;
  uint16_t detDistCm = 0;
};

class RadarLD2401 {
public:
  void begin(HardwareSerial& s, int rxPin, int txPin, uint32_t baud);
  void update(uint32_t now);
  void beginShared(HardwareSerial& s, uint32_t baud);
  void tickShared(uint32_t now) { link_.tick(now); }
  void sharedPortRestarted(uint32_t now) { st_ = S_HDR; hdrIdx_ = tailIdx_ = lenIdx_ = 0; len_ = bodyIdx_ = 0; link_.markRestart(now); }
  void feedByte(uint8_t b, uint32_t now);
  LinkState link(uint32_t now) const { return link_.state(now); }
  const LinkMonitor& stats() const { return link_; }
  const LD2401Data& data() const { return data_; }
  uint32_t baud() const { return baud_; }
  static const char* stateStr(uint8_t st);
private:
  void openPort();
  void parsePayload(const uint8_t* p, uint16_t len, uint32_t now);
  HardwareSerial* ser_ = nullptr;
  int rx_ = -1, tx_ = -1;
  uint32_t baud_ = 256000;
  LinkMonitor link_;
  LD2401Data data_;
  enum { S_HDR, S_LEN, S_BODY, S_TAIL } st_ = S_HDR;
  uint8_t hdrIdx_ = 0, tailIdx_ = 0, lenIdx_ = 0;
  uint16_t len_ = 0, bodyIdx_ = 0;
  uint8_t body_[64] = {};
};
