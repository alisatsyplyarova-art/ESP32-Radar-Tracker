#include "RadarTargets.h"

void RadarTargets::clear() { for (auto& t : t_) t = TrackedTarget(); }

bool RadarTargets::alive(const TrackedTarget& t, uint32_t now) {
  return t.active && (now - t.lastSeenMs) <= TARGET_TIMEOUT_MS;
}

// The sensor reports targets in slots that may be re-ordered from frame to frame.
// To keep a stable ID (and trail/colour) per physical target, every valid measurement
// is matched to the nearest living track within TARGET_MATCH_GATE_MM (greedy, nearest
// pair first). Unmatched measurements start a new track in a free slot.
void RadarTargets::update(const LD2450Target in[3], uint32_t now) {
  int8_t trackOf[3] = {-1, -1, -1};
  bool used[3] = {false, false, false};
  const int32_t gate2 = (int32_t)TARGET_MATCH_GATE_MM * TARGET_MATCH_GATE_MM;

  for (int round = 0; round < 3; round++) {
    int32_t best = gate2 + 1; int bi = -1, bj = -1;
    for (int i = 0; i < 3; i++) {
      if (!in[i].valid || trackOf[i] >= 0) continue;
      for (int j = 0; j < 3; j++) {
        if (used[j] || !alive(t_[j], now)) continue;
        int32_t dx = (int32_t)in[i].x - t_[j].rawX, dy = (int32_t)in[i].y - t_[j].rawY;
        int32_t d2 = dx * dx + dy * dy;
        if (d2 < best) { best = d2; bi = i; bj = j; }
      }
    }
    if (bi < 0) break;
    trackOf[bi] = (int8_t)bj; used[bj] = true;
  }
  for (int i = 0; i < 3; i++) {                       // unmatched -> free slot, else stalest unused one
    if (!in[i].valid || trackOf[i] >= 0) continue;
    int pick = -1;
    for (int j = 0; j < 3 && pick < 0; j++) if (!used[j] && !alive(t_[j], now)) pick = j;
    if (pick < 0) {
      uint32_t oldest = 0;
      for (int j = 0; j < 3; j++)
        if (!used[j] && (pick < 0 || (now - t_[j].lastSeenMs) > oldest)) { pick = j; oldest = now - t_[j].lastSeenMs; }
    }
    if (pick < 0) continue;                           // cannot happen with 3 inputs / 3 tracks
    trackOf[i] = (int8_t)pick; used[pick] = true;
    t_[pick].active = false;                          // force a fresh track (reset smoothing/trail)
  }

  for (int i = 0; i < 3; i++) {
    if (!in[i].valid || trackOf[i] < 0) continue;     // absent slot: aging handled by expire()
    TrackedTarget& t = t_[trackOf[i]];
    bool fresh = !t.active || (now - t.lastSeenMs) > TARGET_TIMEOUT_MS;
    t.rawX = in[i].x; t.rawY = in[i].y; t.speed = in[i].speed;
    t.res = in[i].res; t.rawDist = in[i].dist;
    if (fresh) { t.sx = in[i].x; t.sy = in[i].y; t.trailCount = 0; t.trailHead = 0; }
    else { t.sx += TARGET_SMOOTH * (in[i].x - t.sx); t.sy += TARGET_SMOOTH * (in[i].y - t.sy); }
    t.trail[t.trailHead] = { (int16_t)t.sx, (int16_t)t.sy };
    t.trailHead = (t.trailHead + 1) % TRAIL_LEN;
    if (t.trailCount < TRAIL_LEN) t.trailCount++;
    t.active = true; t.lastSeenMs = now;
  }
}

void RadarTargets::expire(uint32_t now) {
  for (auto& t : t_)
    if (t.active && (now - t.lastSeenMs) > TARGET_TIMEOUT_MS) { t.active = false; t.trailCount = 0; }
}

TargetState RadarTargets::stateOf(uint8_t i, uint32_t now) const {
  const TrackedTarget& t = t_[i];
  if (!t.active) return TS_NONE;
  uint32_t age = now - t.lastSeenMs;
  if (age > TARGET_TIMEOUT_MS) return TS_NONE;
  return age <= TARGET_HOLD_MS ? TS_LIVE : TS_HOLD;
}

uint8_t RadarTargets::liveCount(uint32_t now) const {
  uint8_t n = 0;
  for (uint8_t i = 0; i < 3; i++) if (stateOf(i, now) == TS_LIVE) n++;
  return n;
}

int8_t RadarTargets::nearest(uint32_t now) const {
  int8_t best = -1; float bd = 1e9f;
  for (uint8_t i = 0; i < 3; i++)
    if (stateOf(i, now) == TS_LIVE && t_[i].rawDist < bd) { bd = t_[i].rawDist; best = i; }
  return best;
}

bool RadarTargets::trailAt(const TrackedTarget& t, uint8_t k, TrailPoint& out) {
  if (k >= t.trailCount) return false;
  int idx = ((int)t.trailHead - 1 - (int)k + 2 * TRAIL_LEN) % TRAIL_LEN;
  out = t.trail[idx];
  return true;
}
