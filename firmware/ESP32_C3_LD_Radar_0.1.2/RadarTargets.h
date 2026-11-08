#pragma once
#include <Arduino.h>
#include "Config.h"
#include "RadarLD2450.h"
struct TrailPoint { int16_t x,y; uint16_t atTick; };
static_assert(sizeof(TrailPoint)==6,"TrailPoint must stay compact (6 bytes)");
struct TrackedTarget {
  bool active=false;
  int16_t rawX=0,rawY=0;
  int16_t speed=0;
  uint16_t res=0;
  float rawDist=0;
  float sx=0,sy=0;
  float vx=0,vy=0; // estimated X/Y velocity in mm/s, used for track prediction
  uint32_t lastSeenMs=0;
  uint32_t lastTrailMs=0;
  TrailPoint trail[TRAIL_LEN];
  uint16_t trailCount=0,trailHead=0;
};
enum TargetState:uint8_t { TS_NONE,TS_LIVE,TS_HOLD };
class RadarTargets {
public:
  void clear();
  void update(const LD2450Target in[3],uint32_t now);
  void expire(uint32_t now);
  TargetState stateOf(uint8_t i,uint32_t now) const;
  const TrackedTarget& get(uint8_t i) const {return t_[i];}
  uint8_t liveCount(uint32_t now) const;
  int8_t nearest(uint32_t now) const;
  static bool trailAt(const TrackedTarget& t,uint16_t k,TrailPoint& out);
  static uint32_t trailAgeMs(uint32_t now,const TrailPoint& p) {
    return (uint32_t)(uint16_t)((uint16_t)(now/100UL)-p.atTick)*100UL;
  }
private:
  static bool alive(const TrackedTarget& t,uint32_t now);
  TrackedTarget t_[3];
};
