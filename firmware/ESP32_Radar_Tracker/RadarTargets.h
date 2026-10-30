#pragma once
#include <Arduino.h>
#include "Config.h"
#include "RadarLD2450.h"
struct TrailPoint { int16_t x,y; };
struct TrackedTarget {
 bool active=false; int16_t rawX=0,rawY=0,speed=0; uint16_t res=0; float rawDist=0,sx=0,sy=0;
 uint32_t lastSeenMs=0; TrailPoint trail[TRAIL_LEN]; uint8_t trailCount=0,trailHead=0;
};
enum TargetState:uint8_t { TS_NONE,TS_LIVE,TS_HOLD };
class RadarTargets {
public:
 void clear(); void update(const LD2450Target in[3],uint32_t now); void expire(uint32_t now);
 TargetState stateOf(uint8_t i,uint32_t now) const; const TrackedTarget& get(uint8_t i) const { return t_[i]; }
 uint8_t liveCount(uint32_t now) const; int8_t nearest(uint32_t now) const;
 static bool trailAt(const TrackedTarget& t,uint8_t k,TrailPoint& out);
private: TrackedTarget t_[3];
};