#pragma once
#include <Arduino.h>
#include "Config.h"

enum LinkState : uint8_t { LINK_NO_DATA, LINK_GARBAGE, LINK_OK, LINK_LOST };

inline const char* linkStateStr(LinkState s) {
  switch (s) {
    case LINK_OK: return "OK";
    case LINK_LOST: return "LOST";
    case LINK_GARBAGE: return "BAD DATA";
    default: return "NO DATA";
  }
}

struct LinkMonitor {
  uint32_t bytesRx = 0, framesOk = 0, framesBad = 0, restarts = 0;
  uint32_t startMs = 0, lastFrameMs = 0, lastRestartMs = 0;
  float fps = 0;
  uint32_t fpsCnt = 0, fpsT0 = 0;
  void reset(uint32_t now) { startMs = now; lastRestartMs = now; lastFrameMs = 0; fpsT0 = now; fpsCnt = 0; fps = 0; }
  void frameOk(uint32_t now) { framesOk++; lastFrameMs = now; fpsCnt++; }
  void tick(uint32_t now) { if (now - fpsT0 >= 1000) { fps = fpsCnt * 1000.0f / (float)(now - fpsT0); fpsCnt = 0; fpsT0 = now; } }
  LinkState state(uint32_t now) const {
    if (framesOk == 0) { if (bytesRx == 0) return LINK_NO_DATA; return (now - startMs > LINK_GARBAGE_AFTER_MS) ? LINK_GARBAGE : LINK_NO_DATA; }
    return (now - lastFrameMs <= LINK_TIMEOUT_MS) ? LINK_OK : LINK_LOST;
  }
  bool needRestart(uint32_t now) const { uint32_t ref = startMs; if (lastFrameMs > ref) ref = lastFrameMs; if (lastRestartMs > ref) ref = lastRestartMs; return (now - ref) > RESTART_AFTER_MS; }
  void markRestart(uint32_t now) { restarts++; lastRestartMs = now; }
  uint32_t ageMs(uint32_t now) const { return lastFrameMs ? now - lastFrameMs : 0; }
};