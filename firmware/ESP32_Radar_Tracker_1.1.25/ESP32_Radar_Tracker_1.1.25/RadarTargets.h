#pragma once
#include <Arduino.h>
#include "Config.h"
#include "RadarLD2450.h"

struct TrailPoint { int16_t x, y; };

struct TrackedTarget {
  bool     active = false;     // has a position that is not older than TARGET_TIMEOUT_MS
  int16_t  rawX = 0, rawY = 0; // last measurement as delivered by the sensor (mm)
  int16_t  speed = 0;          // cm/s
  uint16_t res = 0;            // mm
  float    rawDist = 0;        // mm
  float    sx = 0, sy = 0;     // smoothed position, DISPLAY ONLY (mm)
  uint32_t lastSeenMs = 0;
  TrailPoint trail[TRAIL_LEN];
  uint8_t  trailCount = 0, trailHead = 0;
};

enum TargetState : uint8_t { TS_NONE, TS_LIVE, TS_HOLD };

class RadarTargets {
public:
  void clear();
  void update(const LD2450Target in[3], uint32_t now);   // feed one real frame
  void expire(uint32_t now);                             // drop stale targets
  TargetState stateOf(uint8_t i, uint32_t now) const;
  const TrackedTarget& get(uint8_t i) const { return t_[i]; }
  uint8_t liveCount(uint32_t now) const;
  int8_t nearest(uint32_t now) const;                    // index of nearest live target or -1
  static bool trailAt(const TrackedTarget& t, uint8_t k, TrailPoint& out); // k=0 newest
private:
  static bool alive(const TrackedTarget& t, uint32_t now);
  TrackedTarget t_[3];
};
