#pragma once
#include <Arduino.h>
#include "LinkMonitor.h"

// HLK-LD2450 24 GHz multi-target tracking radar, UART 256000 8N1.
// Data frame (30 bytes): AA FF 03 00 | 3 x 8 bytes | 55 CC
// Target (little endian): X int16, Y int16, speed int16, distance resolution uint16.
// Signed fields: bit15 = 1 -> positive, bit15 = 0 -> negative, magnitude = low 15 bits.
// X, Y in mm; speed in cm/s. An all-zero target slot = no target.
// The frame has NO checksum: validity = header + tail + plausibility of values.
struct LD2450Target {
  bool     valid = false;
  int16_t  x = 0;      // mm (negative = left)
  int16_t  y = 0;      // mm (forward)
  int16_t  speed = 0;  // cm/s as reported by the sensor
  uint16_t res = 0;    // mm, distance resolution reported by the sensor
  float    dist = 0;   // mm, sqrtf(x^2 + y^2) computed here
};

class RadarLD2450 {
public:
  static const uint8_t kFrameLen = 30;

  void begin(HardwareSerial& s, int rxPin, int txPin, uint32_t baud);
  void update(uint32_t now);                       // non-blocking, call every loop
  void beginShared(HardwareSerial& s, uint32_t baud);
  void sharedPortRestarted(uint32_t now);          // call after main.cpp re-opened the shared UART
  void tickShared(uint32_t now);                   // config sequence + timers; never reopens the shared port
  void feedByte(uint8_t b, uint32_t now);          // parser entry (also used by host test)
  bool consumeFrame(LD2450Target out[3]);          // true once per new valid frame

  LinkState link(uint32_t now) const { return link_.state(now); }
  const LinkMonitor& stats() const { return link_; }
  uint32_t rejected() const { return rejected_; }
  uint32_t baud() const { return baud_; }
  static int16_t decodeSigned(uint16_t raw) {
    int16_t m = (int16_t)(raw & 0x7FFF);
    return (raw & 0x8000) ? m : (int16_t)-m;
  }

private:
  void openPort();
  void parseFrame(uint32_t now);
  void sendCmd(uint16_t cmd, const uint8_t* val, uint8_t vlen);
  void runConfig(uint32_t now);

  HardwareSerial* ser_ = nullptr;
  int rx_ = -1, tx_ = -1;
  uint32_t baud_ = 256000;
  uint8_t buf_[kFrameLen];
  uint8_t idx_ = 0;
  LinkMonitor link_;
  LD2450Target pending_[3];
  bool frameNew_ = false;
  uint32_t rejected_ = 0;
  uint8_t cfgStep_ = 0;
  uint32_t cfgNext_ = 0;
};
