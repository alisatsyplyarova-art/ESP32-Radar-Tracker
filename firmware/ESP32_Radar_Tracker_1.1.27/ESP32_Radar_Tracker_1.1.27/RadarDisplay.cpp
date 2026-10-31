#include "RadarDisplay.h"
#include <stdarg.h>
#include <math.h>

static const uint16_t kRangesMm[7] = {500, 1000, 1500, 2000, 3000, 4000, 6000};
static const uint8_t  kBrightPwm[5] = {15, 50, 110, 190, 255};
static const uint16_t kAlertMm[4] = {0, 1000, 2000, 3000};
static const float    kDeg = 0.017453293f;
static const int W = 240, H = 135;

uint16_t RadarDisplay::rangeMm(uint8_t i) { return kRangesMm[i % 7]; }

bool RadarDisplay::alertActive(const UiContext& c) const {
  uint16_t thr = kAlertMm[c.set->alertIdx % 4];
  if (!thr) return false;
  for (uint8_t i = 0; i < 3; i++)
    if (c.targets->stateOf(i, c.now) == TS_LIVE && c.targets->get(i).rawDist < thr) return true;
  // LD2401 (presence/distance only): alert if a detection is inside the zone
  if (c.presValid && c.pres.state) {
    uint16_t cm = (c.pres.state == 2) ? c.pres.statDistCm : (c.pres.movDistCm ? c.pres.movDistCm : c.pres.statDistCm);
    if (cm && (uint32_t)cm * 10 < thr) return true;
  }
  return false;
}

bool RadarDisplay::begin() {
  tft_.init();
  tft_.setRotation(1);                       // landscape: 90 degrees counter-clockwise from the previous 180-degree orientation
  tft_.fillScreen(TFT_BLACK);

  // Backlight ON first: whatever fails later, the panel must never stay dark.
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // Boot splash drawn straight to the panel (no sprite). If you see this text, the
  // TFT_eSPI setup, SPI wiring and backlight are fine.
  tft_.setTextColor(TFT_WHITE, TFT_BLACK);
  tft_.drawString("ESP32 Radar Tracker", 8, 8, 2);
  tft_.drawString("display OK, starting...", 8, 30, 2);
#ifdef USER_SETUP_INFO
  Serial.printf("[UI] TFT_eSPI setup: %s\n", USER_SETUP_INFO);
#endif
  Serial.printf("[UI] panel %dx%d, BL pin %d\n", (int)TFT_WIDTH, (int)TFT_HEIGHT, (int)TFT_BL);

  // Colours and themes are set BEFORE the sprite is allocated, so a failed allocation
  // can no longer leave them uninitialised (all-black colours).
  th_[0] = { tft_.color565(0, 255, 80),   tft_.color565(70, 150, 95),  tft_.color565(35, 95, 55),  "GREEN" };
  th_[1] = { tft_.color565(255, 150, 0),  tft_.color565(190, 125, 35), tft_.color565(100, 65, 20), "AMBER" };
  th_[2] = { tft_.color565(0, 210, 255),  tft_.color565(0, 70, 100),  tft_.color565(0, 120, 160), "CYAN"  };
  cBg_ = tft_.color565(0, 0, 0);
  cText_ = tft_.color565(245, 245, 245);
  cWarn_ = tft_.color565(255, 190, 0);
  cBad_ = tft_.color565(255, 60, 60);
  cGray_ = tft_.color565(150, 150, 150);
  cGood_ = tft_.color565(60, 255, 120);
  cSlot_[0] = tft_.color565(80, 255, 255);
  cSlot_[1] = tft_.color565(255, 230, 60);
  cSlot_[2] = tft_.color565(255, 110, 220);

  // Arduino-ESP32 core 3.x: LEDC is attached directly to the GPIO pin.
  backlightPwmReady_ = ledcAttach(TFT_BL, 5000, 8);
  if (!backlightPwmReady_) {
    // Keep the display usable even if LEDC cannot claim the backlight pin.
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    Serial.println(F("[UI] LEDC backlight attach failed; using full brightness"));
  }

  spr_.setColorDepth(16);
  if (!spr_.createSprite(W, H)) {
    Serial.printf("[UI] sprite alloc failed: need %u bytes, free heap %u, largest block %u\n",
                  (unsigned)(W * H * 2), (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());
    tft_.setTextColor(TFT_RED, TFT_BLACK);
    tft_.drawString("SPRITE ALLOC FAILED", 8, 56, 2);
    tft_.drawString("not enough free RAM", 8, 78, 2);
    return false;
  }
  spriteReady_ = true;
  Serial.printf("[UI] sprite OK (%u bytes), free heap %u\n", (unsigned)(W * H * 2), (unsigned)ESP.getFreeHeap());
  return true;
}

void RadarDisplay::setBrightness(uint8_t idx) {
  const uint8_t duty = kBrightPwm[idx % 5];
  if (backlightPwmReady_) ledcWrite(TFT_BL, duty);
  else { pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, duty >= 128 ? HIGH : LOW); }
}

uint16_t RadarDisplay::scale(uint16_t c, uint8_t f) const {
  uint16_t r = ((c >> 11) & 31) * f / 255, g = ((c >> 5) & 63) * f / 255, b = (c & 31) * f / 255;
  return (r << 11) | (g << 5) | b;
}

void RadarDisplay::txt(int x, int y, uint8_t font, uint16_t col, const char* fmt, ...) {
  char b[48]; va_list a; va_start(a, fmt); vsnprintf(b, sizeof(b), fmt, a); va_end(a);
  spr_.setTextColor(col);                    // single colour = transparent background
  spr_.setTextDatum(TL_DATUM);
  spr_.drawString(b, x, y, font);
}
void RadarDisplay::txtC(int x, int y, uint8_t font, uint16_t col, const char* fmt, ...) {
  char b[48]; va_list a; va_start(a, fmt); vsnprintf(b, sizeof(b), fmt, a); va_end(a);
  spr_.setTextColor(col);
  spr_.setTextDatum(TC_DATUM);
  spr_.drawString(b, x, y, font);
}

uint16_t RadarDisplay::linkCol(LinkState s) const {
  switch (s) { case LINK_OK: return cGood_; case LINK_LOST: return cWarn_; case LINK_GARBAGE: return cBad_; default: return cGray_; }
}

void RadarDisplay::bar(int x, int y, int w, int h, int val, int maxv, uint16_t col) {
  spr_.drawRect(x, y, w, h, scale(col, 120));
  int fw = constrain(val, 0, maxv) * (w - 2) / maxv;
  if (fw > 0) spr_.fillRect(x + 1, y + 1, fw, h - 2, col);
}

void RadarDisplay::header(const UiContext& c, const char* title) {
  const Theme& t = th_[c.set->theme];
  bool al = alertActive(c);
  bool flash = al && ((c.now / 250) & 1);
  spr_.fillRect(0, 0, W, 16, flash ? scale(cBad_, 140) : scale(t.accent, 50));
  txt(3, 0, 2, al ? cBad_ : t.accent, "%s", al ? "! ALERT" : title);
  uint32_t n = c.now;
  spr_.fillCircle(104, 8, 3, linkCol(c.ld2450->link(n)));    // real link state, also in demo
  spr_.fillCircle(118, 8, 3, linkCol(c.ld2401->link(n)));
}

void RadarDisplay::footer(const UiContext& c) {
  const Theme& t = th_[c.set->theme];
  for (int i = 0; i < SCR_COUNT; i++) {
    int x = 6 + i * 9;
    if (i == c.screen) spr_.fillCircle(x, 232, 3, t.accent); else spr_.drawCircle(x, 232, 2, t.dim);
  }
  txt(84, 227, 1, t.dim, "UI %2.0ffps", c.uiFps);
}

// Sector radar field (+-60 deg), origin (ox,oy), radius R pixels
void RadarDisplay::field(const UiContext& c, int ox, int oy, int R) {
  const Theme& t = th_[c.set->theme];
  const uint16_t radarCyan = tft_.color565(0, 210, 255);
  const uint16_t targetOrange = tft_.color565(255, 140, 0);
  const uint16_t trailBlue = tft_.color565(0, 90, 255);
  const uint32_t rng = rangeMm(c.set->rangeIdx);

  // sweep beam with fading tail (decorative, not a measurement)
  float ph = (c.now % SWEEP_PERIOD_MS) / (float)SWEEP_PERIOD_MS;
  float a0 = -60.0f + 120.0f * ph;
  for (int j = 0; j < 9; j++) {
    float a = a0 - j * 2.5f; if (a < -60.0f) break;
    spr_.drawLine(ox, oy, ox + (int)(R * sinf(a * kDeg)), oy - (int)(R * cosf(a * kDeg)), scale(radarCyan, 200 - j * 22));
  }
  // range rings
  for (int k = 1; k <= 4; k++) {
    float r = R * k / 4.0f;
    int px = ox + (int)(r * sinf(-60 * kDeg)), py = oy - (int)(r * cosf(-60 * kDeg));
    for (int a = -57; a <= 60; a += 3) {
      int nx = ox + (int)(r * sinf(a * kDeg)), ny = oy - (int)(r * cosf(a * kDeg));
      if (c.set->alien && k != 4) spr_.drawPixel(nx, ny, radarCyan);           // dotted rings
      else spr_.drawLine(px, py, nx, ny, radarCyan);
      px = nx; py = ny;
    }
    if (R >= 50) txt(ox + 2, oy - (int)r + 1, 1, t.dim, "%.1f", rng * k / 4000.0f);
  }
  // angular marks
  for (int a = -60; a <= 60; a += 30) {
    int ex = ox + (int)(R * sinf(a * kDeg)), ey = oy - (int)(R * cosf(a * kDeg));
    spr_.drawLine(ox, oy, ex, ey, radarCyan);
  }
  spr_.fillCircle(ox, oy, 2, t.accent);
  if (c.set->alien)                                                      // tick marks every 10 deg
    for (int a = -60; a <= 60; a += 10) {
      float s = sinf(a * kDeg), co = cosf(a * kDeg);
      spr_.drawLine(ox + (int)((R - 3) * s), oy - (int)((R - 3) * co), ox + (int)(R * s), oy - (int)(R * co), radarCyan);
    }
  // alert zone (dashed red arc)
  uint16_t thr = kAlertMm[c.set->alertIdx % 4];
  if (thr && thr < rng) {
    float r = (float)R * thr / rng;
    for (int a = -60; a <= 60; a += 3)
      spr_.drawPixel(ox + (int)(r * sinf(a * kDeg)), oy - (int)(r * cosf(a * kDeg)), scale(cBad_, 200));
  }

  // targets (real or demo - decided by caller)
  const RadarTargets& tg = *c.targets;
  float k = (float)R / rng;
  for (uint8_t i = 0; i < 3; i++) {
    TargetState st = tg.stateOf(i, c.now);
    if (st == TS_NONE) continue;
    const TrackedTarget& tt = tg.get(i);
    uint16_t col = targetOrange; // orange target marker
    bool hit = thr && st == TS_LIVE && tt.rawDist < thr;
    // Target markers stay orange; the warning zone remains separately highlighted.
    float delta = 0;                                   // phosphor fade: bright right after the beam passes
    if (c.set->alien) {
      float th = constrain(atan2f(tt.sx, tt.sy) / kDeg, -60.0f, 60.0f);
      delta = a0 - th; if (delta < 0) delta += 120.0f;
      col = scale(col, (uint8_t)(255 - delta * 1.7f));
    }
    if (c.set->trails) {
      for (uint8_t j = 1; j < TRAIL_LEN; j++) {
        TrailPoint p; if (!RadarTargets::trailAt(tt, j, p)) break;
        int tx = ox + (int)(p.x * k), ty = oy - (int)(p.y * k);
        if (tx >= 0 && tx < W && ty >= 0 && ty < H && hypotf(tx - ox, ty - oy) <= R)
          spr_.fillCircle(tx, ty, 1, scale(tft_.color565(0, 90, 255), 200 - j * 24));
      }
    }
    float dx = tt.sx * k, dy = -tt.sy * k;
    bool clamped = false;
    float d = sqrtf(dx * dx + dy * dy);
    if (d > R) { dx *= R / d; dy *= R / d; clamped = true; }
    int px = constrain(ox + (int)dx, 4, W - 5), py = constrain(oy + (int)dy, 20, H - 20);
    if (st == TS_HOLD || clamped) spr_.drawCircle(px, py, 3, st == TS_HOLD ? scale(col, 110) : col);  // dim = coasting, hollow = off-range
    else {
      spr_.fillCircle(px, py, 3, col); spr_.drawCircle(px, py, 5, scale(col, 120));
      if (c.set->alien && delta < 12.0f) spr_.drawCircle(px, py, 8, scale(col, 160));   // ping when beam hits
    }
    txt(px + 6, py - 8, 1, hit ? cBad_ : cSlot_[i], "%d", i + 1);
  }
}

void RadarDisplay::render(const UiContext& c) {
  // begin() may fail to allocate the framebuffer. Keep the diagnostic splash
  // visible instead of drawing/pushing through an invalid sprite.
  if (!spriteReady_ || !c.set || !c.targets || !c.ld2450 || !c.ld2401) return;
  const Theme& th = th_[c.set->theme % 3];
  spr_.fillSprite(cBg_);
  spr_.fillRect(0, 0, W, 17, scale(th.accent, 45));
  const char* title = c.screen == SCR_RADAR ? (c.set->alien ? "TRACKER" : "RADAR") :
                      c.screen == SCR_TARGETS ? "TARGETS" :
                      c.screen == SCR_PRESENCE ? "PRESENCE" :
                      c.screen == SCR_DUAL ? "SENSOR" :
                      c.screen == SCR_DIAG ? "DIAGNOSTICS" : "SETTINGS";
  txt(4, 1, 2, th.accent, "%s", title);
  // Fixed status indicators; never write a second title over this row.
  spr_.fillCircle(211, 8, 3, linkCol(c.ld2450->link(c.now)));
  spr_.fillCircle(226, 8, 3, linkCol(c.ld2401->link(c.now)));

  if (c.screen == SCR_PRESENCE || (c.screen == SCR_RADAR && c.presValid && c.ld2401->link(c.now) == LINK_OK && c.ld2450->link(c.now) != LINK_OK)) {
    // LD2401 interface based on the user's v1.0.004: fan-shaped 0..6 m range
    // rings with a 60-second distance history on the centreline. LD2401 has no bearing.
    struct HistPoint { int16_t cm; uint8_t state; };
    static HistPoint hist[60];
    static uint8_t histHead = 0, histCount = 0;
    static uint32_t lastHistSample = 0;
    const LD2401Data& d = c.pres;
    const uint16_t radarCyan = tft_.color565(0, 210, 255);
    const uint16_t targetOrange = tft_.color565(255, 140, 0);
    const uint16_t trailBlue = trailBlue;
    bool fresh = c.presValid && c.ld2401->link(c.now) == LINK_OK;
    if (c.now - lastHistSample >= 1000 || histCount == 0) {
      lastHistSample = c.now;
      int cm = -1;
      if (fresh && d.state >= 1 && d.state <= 3) {
        // Match the proven v1.0.004 behavior: select the distance channel
        // according to the LD2401 state, rather than preferring detDistCm.
        // State 1 = moving, 2 = stationary, 3 = both (prefer moving if present).
        uint16_t v = 0;
        if (d.state == 1) v = d.movDistCm;
        else if (d.state == 2) v = d.statDistCm;
        else if (d.state == 3) v = d.movDistCm > 0 ? d.movDistCm : d.statDistCm;
        if (v > 0 && v <= 600) cm = v;
      }
      hist[histHead] = {(int16_t)cm, d.state};
      histHead = (histHead + 1) % 60;
      if (histCount < 60) histCount++;
    }
    // Main title already drawn by render(); keep subtitle on its own row.
    spr_.fillRect(0, 17, W, 10, cBg_);
    txt(4, 18, 1, th.accent, "LD2401  RANGE %g m", rangeMm(c.set->rangeIdx) / 1000.0f);
    const int ox = 132, oy = 105, R = 77;
    // fan outline and range arcs: decorative coverage only, not measured bearing
    for (int side = -1; side <= 1; side += 2) {
      float a = side * 60.0f * kDeg;
      spr_.drawLine(ox, oy, ox + (int)(R * sinf(a)), oy - (int)(R * cosf(a)), radarCyan);
    }
    for (int k = 1; k <= 4; ++k) {
      int rr = R * k / 4;
      int px = ox + (int)(rr * sinf(-60 * kDeg));
      int py = oy - (int)(rr * cosf(-60 * kDeg));
      for (int a = -57; a <= 60; a += 3) {
        int nx = ox + (int)(rr * sinf(a * kDeg));
        int ny = oy - (int)(rr * cosf(a * kDeg));
        spr_.drawLine(px, py, nx, ny, radarCyan); px = nx; py = ny;
      }
    }
    const uint16_t maxCm = rangeMm(c.set->rangeIdx) / 10;
    for (int k = 1; k <= 4; ++k) {
      char label[12]; float m = ((float)maxCm / 100.0f) * k / 4.0f;
      snprintf(label, sizeof(label), "%.2g", m);
      int rr = R * k / 4; int lx = ox - (int)(rr * 0.72f) - 7; int ly = oy - (int)(rr * 0.72f) - 4;
      spr_.fillRect(lx, ly, 24, 9, cBg_); spr_.setTextColor(th.accent); spr_.setTextDatum(TL_DATUM); spr_.drawString(label, lx, ly, 1);
    }
    // oldest history points are dim; latest point is bright. All stay on centreline.
    int oldest = (histHead - histCount + 60) % 60;
    for (int i = 0; i < histCount; ++i) {
      HistPoint h = hist[(oldest + i) % 60];
      if (h.cm < 0 || h.cm > maxCm) continue;
      int rr = 4 + ((int)h.cm * (R - 4)) / maxCm;
      uint16_t col = (i == histCount - 1) ? tft_.color565(255, 140, 0) : tft_.color565(0, 90, 255);
      spr_.fillCircle(ox, oy - rr, i == histCount - 1 ? 3 : 2, col);
    }
    int cm = -1;
    if (fresh && d.state >= 1 && d.state <= 3) {
      if (d.state == 1) cm = d.movDistCm;
      else if (d.state == 2) cm = d.statDistCm;
      else if (d.state == 3) cm = d.movDistCm > 0 ? d.movDistCm : d.statDistCm;
      if (cm > maxCm) cm = -1;
    }
    if (cm > 0 && cm <= maxCm) {
      int yy = oy - (4 + (cm * (R - 4)) / maxCm);
      uint16_t col = tft_.color565(255, 140, 0);
      spr_.drawFastHLine(ox - 7, yy, 15, col); spr_.fillCircle(ox, yy, 3, col);
    }
    spr_.drawLine(ox, oy, ox, oy - R, radarCyan);
    spr_.fillCircle(ox, oy, 3, th.accent);
    txtC(132, 109, 1, th.accent, "RADAR");
    // Compact footer follows v1.0.004 and remains readable in landscape.
    uint16_t statusCol = !fresh ? cBad_ : (d.state == 0 ? th.dim : (d.state == 2 ? cSlot_[0] : cWarn_));
    const char* status = !fresh ? "NO DATA" : (d.state == 0 ? "NO TARGET" : (d.state == 1 ? "MOVING" : (d.state == 2 ? "STILL" : "MOVE+STILL")));
    spr_.fillRoundRect(3, 118, 76, 15, 3, statusCol);
    spr_.setTextColor(cBg_); spr_.setTextDatum(TC_DATUM); spr_.drawString(status, 41, 121, 1);
    if (cm > 0 && cm <= maxCm) txt(84, 120, 2, cText_, "%d.%02d m", cm / 100, cm % 100);
    else txt(84, 120, 2, cText_, "--.-- m");
    txt(196, 121, 1, th.dim, "E%u", fresh ? (d.state == 2 ? d.statEnergy : d.movEnergy) : 0);
  } else if (c.screen == SCR_TARGETS) {
    const RadarTargets& tg = *c.targets;
    txt(5, 21, 2, linkCol(c.ld2450->link(c.now)), "LD2450 %s", linkStateStr(c.ld2450->link(c.now)));
    for (uint8_t i = 0; i < 3; i++) {
      int y = 44 + i * 27;
      TargetState st = tg.stateOf(i, c.now); const TrackedTarget& tt = tg.get(i);
      txt(5, y, 2, cSlot_[i], "T%d", i + 1);
      if (st == TS_NONE) txt(48, y, 2, th.dim, "No target");
      else txt(48, y, 2, st == TS_LIVE ? cGood_ : cWarn_, "X%+.2f Y%+.2f  D%.2fm", tt.rawX/1000.0f, tt.rawY/1000.0f, tt.rawDist/1000.0f);
    }
    txt(5, 123, 1, th.dim, "B1 screen  B2 range | hold B1 diagnostics");
  } else if (c.screen == SCR_DIAG) {
    txt(5, 25, 2, linkCol(c.ld2450->link(c.now)), "LD2450 %s", linkStateStr(c.ld2450->link(c.now)));
    txt(5, 50, 2, linkCol(c.ld2401->link(c.now)), "LD2401 %s", linkStateStr(c.ld2401->link(c.now)));
    txt(5, 80, 2, cText_, "UI %.0f fps   Heap %u k", c.uiFps, (unsigned)(ESP.getFreeHeap()/1024));
    txt(5, 108, 1, th.dim, "Open Serial Monitor for UART counters");
  } else if (c.screen == SCR_SETTINGS) {
    // Six settings fit the 240x135 landscape panel. The selected row is always visible.
    const UiSettings& s = *c.set;
    char values[kItems][16];
    snprintf(values[0], sizeof(values[0]), "%.1fm", rangeMm(s.rangeIdx) / 1000.0f);
    snprintf(values[1], sizeof(values[1]), "%u/5", s.brightIdx + 1);
    snprintf(values[2], sizeof(values[2]), "%s", th_[s.theme].name);
    snprintf(values[3], sizeof(values[3]), "%s", s.alien ? "ALIEN" : "CLASSIC");
    if (kAlertMm[s.alertIdx % 4]) snprintf(values[4], sizeof(values[4]), "%.1fm", kAlertMm[s.alertIdx % 4] / 1000.0f);
    else snprintf(values[4], sizeof(values[4]), "OFF");
    snprintf(values[5], sizeof(values[5]), "%s", s.trails ? "ON" : "OFF");
    const char* names[kItems] = {"Range", "Bright", "Theme", "Style", "Alert", "Trails"};
    for (int i = 0; i < kItems; ++i) {
      int y = 20 + i * 16;
      if (i == s.item) {
        spr_.fillRoundRect(2, y - 1, W - 4, 15, 2, scale(th.accent, 45));
        spr_.drawRoundRect(2, y - 1, W - 4, 15, 2, th.accent);
      }
      txt(7, y, 1, cText_, "%s", names[i]);
      txt(102, y, 1, i == s.item ? th.accent : cText_, "%s", values[i]);
    }
    txt(174, 22, 1, th.dim, "B1 hold: next");
    txt(174, 34, 1, th.dim, "item");
    txt(174, 54, 1, th.dim, "B2: change");
    txt(174, 66, 1, th.dim, "value");
    txt(174, 91, 1, c.demo ? cWarn_ : th.dim, "DEMO %s", c.demo ? "ON" : "OFF");
  } else if (c.screen == SCR_DUAL) {
    // Compact dual-sensor view; sensor measurements remain independent (not fused).
    const RadarTargets& tg = *c.targets;
    int8_t nr = tg.nearest(c.now);
    txt(5, 24, 2, linkCol(c.ld2450->link(c.now)), "LD2450 %s", linkStateStr(c.ld2450->link(c.now)));
    txt(5, 46, 1, cText_, "Targets %d/3", tg.liveCount(c.now));
    if (nr >= 0) txt(112, 46, 1, cSlot_[nr], "Nearest %.2fm", tg.get(nr).rawDist / 1000.0f);
    txt(5, 70, 2, linkCol(c.ld2401->link(c.now)), "LD2401 %s", linkStateStr(c.ld2401->link(c.now)));
    if (c.presValid) {
      txt(5, 94, 2, c.pres.state ? cWarn_ : cGood_, "%s", RadarLD2401::stateStr(c.pres.state));
      txt(112, 96, 2, cText_, "%ucm", c.pres.detDistCm);
    } else txt(5, 96, 2, th.dim, "No presence data");
    txt(5, 120, 1, th.dim, "Sensors are independent; LD2401 has no X/Y");
  } else {
    // Landscape overview: radar graphic on the left, large readable status on the right.
    field(c, 58, 105, 57);
    txt(124, 25, 2, cText_, "Targets %d/3", c.targets->liveCount(c.now));
    txt(124, 49, 2, linkCol(c.ld2450->link(c.now)), "LD2450 %s", linkStateStr(c.ld2450->link(c.now)));
    txt(124, 73, 2, linkCol(c.ld2401->link(c.now)), "LD2401 %s", linkStateStr(c.ld2401->link(c.now)));
    if (c.presValid) txt(124, 96, 2, c.pres.state ? cWarn_ : cGood_, "%s %ucm", RadarLD2401::stateStr(c.pres.state), c.pres.detDistCm);
    else txt(124, 96, 2, th.dim, "Presence --");
    if (c.demo) txt(124, 117, 1, cWarn_, "DEMO DATA");
    else txt(124, 117, 1, th.dim, "B1 screen / B2 range");
  }
  spr_.pushSprite(0, 0);
}

// ----------------------------------------------------------------------------
void RadarDisplay::scrRadar(const UiContext& c) {
  const Theme& t = th_[c.set->theme];
  header(c, c.set->alien ? "TRACKER" : "RADAR");
  field(c, 67, 104, 76);
  if (c.demo) txtC(67, 18, 1, ((c.now / 500) & 1) ? cWarn_ : cBad_, "DEMO DATA");
  txt(2, 108, 1, t.dim, "#  X(m)  Y(m)  D(m)");
  const RadarTargets& tg = *c.targets;
  for (uint8_t i = 0; i < 3; i++) {
    TargetState st = tg.stateOf(i, c.now); const TrackedTarget& tt = tg.get(i);
    if (st == TS_NONE) txt(2, 118 + i * 10, 1, t.dim, "%d  ---", i + 1);
    else txt(2, 118 + i * 10, 1, st == TS_LIVE ? cSlot_[i] : scale(cSlot_[i], 110), "%d %+.2f %5.2f %5.2f",
             i + 1, tt.rawX / 1000.0f, tt.rawY / 1000.0f, tt.rawDist / 1000.0f);
  }
  spr_.drawFastHLine(0, 150, W, t.dim);
  uint32_t n = c.now;
  LinkState l1 = c.ld2450->link(n), l2 = c.ld2401->link(n);
  txt(2, 154, 1, linkCol(l1), "LD2450 %s %.0ffps", linkStateStr(l1), c.ld2450->stats().fps);
  txt(2, 165, 1, linkCol(l2), "LD2401 %s", linkStateStr(l2));
  if (c.presValid) txt(2, 176, 1, c.pres.state ? cWarn_ : cText_, "  %s %ucm", RadarLD2401::stateStr(c.pres.state), c.pres.detDistCm);
  else txt(2, 176, 1, t.dim, "  no presence data");
  txt(2, 192, 1, cText_, "TGT %d/3  RNG %.1fm", tg.liveCount(c.now), RadarDisplay::rangeMm(c.set->rangeIdx) / 1000.0f);
  txt(2, 206, 1, t.dim, "B1 screen  B2 range");
  txt(2, 216, 1, t.dim, "B1 hold diag B2 demo");
}

void RadarDisplay::scrTargets(const UiContext& c) {
  const Theme& t = th_[c.set->theme];
  header(c, c.demo ? "TARGETS DEMO" : "TARGETS");
  const RadarTargets& tg = *c.targets;
  for (uint8_t i = 0; i < 3; i++) {
    int y0 = 18 + i * 69;
    spr_.drawRect(1, y0, W - 2, 66, t.dim);
    TargetState st = tg.stateOf(i, c.now); const TrackedTarget& tt = tg.get(i);
    uint16_t col = cSlot_[i];
    txt(5, y0 + 3, 2, col, "T%d", i + 1);
    txt(40, y0 + 6, 1, st == TS_LIVE ? cGood_ : (st == TS_HOLD ? cWarn_ : t.dim), "%s", st == TS_LIVE ? "TRACK" : (st == TS_HOLD ? "HOLD (no data)" : "no target"));
    if (st == TS_NONE) continue;
    float ang = atan2f((float)tt.rawX, (float)tt.rawY) / kDeg;
    txt(5, y0 + 20, 2, cText_, "X%+.2f Y%.2f", tt.rawX / 1000.0f, tt.rawY / 1000.0f);
    txt(5, y0 + 36, 2, cText_, "D%.2fm A%+.0f", tt.rawDist / 1000.0f, ang);
    const char* dir = tt.speed == 0 ? "STILL" :
#if LD2450_POSITIVE_SPEED_IS_AWAY
        (tt.speed > 0 ? "AWAY" : "NEAR");
#else
        (tt.speed > 0 ? "NEAR" : "AWAY");
#endif
    txt(5, y0 + 54, 1, t.accent, "V%+dcm/s %s res%u", tt.speed, dir, tt.res);
  }
}

void RadarDisplay::scrPresence(const UiContext& c) {
  const Theme& t = th_[c.set->theme];
  header(c, c.demo ? "PRESENCE DEMO" : "PRESENCE");
  LinkState l = c.ld2401->link(c.now);
  txt(3, 20, 1, linkCol(l), "LD2401 link: %s", linkStateStr(l));
  if (!c.presValid) {
    txtC(67, 60, 4, cBad_, "NO DATA");
    txt(3, 100, 1, t.dim, "no valid frames from");
    txt(3, 110, 1, t.dim, "the presence sensor");
    return;
  }
  const LD2401Data& d = c.pres;
  txtC(67, 40, 4, d.state ? cWarn_ : cGood_, "%s", RadarLD2401::stateStr(d.state));
  txt(3, 82, 1, cText_, "Moving  %3ucm  E%3u", d.movDistCm, d.movEnergy);
  bar(3, 93, 129, 8, d.movEnergy, 100, cWarn_);
  txt(3, 108, 1, cText_, "Static  %3ucm  E%3u", d.statDistCm, d.statEnergy);
  bar(3, 119, 129, 8, d.statEnergy, 100, t.accent);
  txt(3, 136, 1, cText_, "Detect  %3ucm", d.detDistCm);
  txt(3, 156, 1, t.dim, "1-D distance only:");
  txt(3, 166, 1, t.dim, "no X/Y coordinates");
  txt(3, 176, 1, t.dim, "from this sensor");
  txt(3, 196, 1, t.dim, "frame age %lums", (unsigned long)c.ld2401->stats().ageMs(c.now));
}

void RadarDisplay::scrDual(const UiContext& c) {
  const Theme& t = th_[c.set->theme];
  header(c, c.demo ? "DUAL DEMO" : "DUAL VIEW");
  field(c, 67, 88, 56);
  const RadarTargets& tg = *c.targets;
  uint32_t n = c.now;
  LinkState l1 = c.ld2450->link(n), l2 = c.ld2401->link(n);
  spr_.drawFastHLine(0, 94, W, t.dim);
  txt(2, 98, 1, linkCol(l1), "LD2450 %s", linkStateStr(l1));
  int8_t nr = tg.nearest(n);
  if (nr >= 0) txt(2, 108, 1, cText_, "%d tgt, nearest %.2fm", tg.liveCount(n), tg.get(nr).rawDist / 1000.0f);
  else txt(2, 108, 1, t.dim, "0 targets");
  spr_.drawFastHLine(0, 122, W, t.dim);
  txt(2, 126, 1, linkCol(l2), "LD2401 %s", linkStateStr(l2));
  if (c.presValid) {
    txt(2, 136, 1, c.pres.state ? cWarn_ : cText_, "%s %ucm", RadarLD2401::stateStr(c.pres.state), c.pres.detDistCm);
    uint16_t rng = rangeMm(c.set->rangeIdx);
    bar(2, 150, 131, 8, c.pres.detDistCm * 10, rng, c.pres.state ? cWarn_ : t.accent);
    txt(2, 161, 1, t.dim, "0        %.1fm (1-D)", rng / 1000.0f);
  } else txt(2, 136, 1, t.dim, "no presence data");
  txt(2, 182, 1, t.dim, "Sensors are NOT fused:");
  txt(2, 192, 1, t.dim, "LD2401 has no X/Y.");
  if (c.demo) txtC(67, 18, 1, ((c.now / 500) & 1) ? cWarn_ : cBad_, "DEMO DATA");
}

void RadarDisplay::scrDiag(const UiContext& c) {
  const Theme& t = th_[c.set->theme];
  header(c, "DIAGNOSTICS");
  uint32_t n = c.now; int y = 19;
  const RadarLD2450& a = *c.ld2450; const RadarLD2401& b = *c.ld2401;
  LinkState la = a.link(n), lb = b.link(n);
  txt(2, y, 1, t.accent, "-- LD2450 (UART1) --"); y += 10;
  txt(2, y, 1, linkCol(la), "link %s", linkStateStr(la)); y += 10;
  txt(2, y, 1, cText_, "%lu 8N1 RX%d TX%d", (unsigned long)a.baud(), PIN_LD2450_RX, PIN_LD2450_TX); y += 10;
  txt(2, y, 1, cText_, "bytes %lu", (unsigned long)a.stats().bytesRx); y += 10;
  txt(2, y, 1, cText_, "ok %lu bad %lu rej %lu", (unsigned long)a.stats().framesOk, (unsigned long)a.stats().framesBad, (unsigned long)a.rejected()); y += 10;
  txt(2, y, 1, cText_, "%.1ffps age %lums rst %lu", a.stats().fps, (unsigned long)a.stats().ageMs(n), (unsigned long)a.stats().restarts); y += 14;
  txt(2, y, 1, t.accent, "-- LD2401 (UART2) --"); y += 10;
  txt(2, y, 1, linkCol(lb), "link %s", linkStateStr(lb)); y += 10;
  txt(2, y, 1, cText_, "%lu 8N1 RX%d TX%d", (unsigned long)b.baud(), PIN_LD2401_RX, PIN_LD2401_TX); y += 10;
  txt(2, y, 1, cText_, "bytes %lu", (unsigned long)b.stats().bytesRx); y += 10;
  txt(2, y, 1, cText_, "ok %lu bad %lu", (unsigned long)b.stats().framesOk, (unsigned long)b.stats().framesBad); y += 10;
  txt(2, y, 1, cText_, "%.1ffps age %lums rst %lu", b.stats().fps, (unsigned long)b.stats().ageMs(n), (unsigned long)b.stats().restarts); y += 14;
  txt(2, y, 1, t.accent, "-- SYSTEM --"); y += 10;
  txt(2, y, 1, cText_, "ui %.0ffps heap %uk", c.uiFps, (unsigned)(ESP.getFreeHeap() / 1024)); y += 10;
  txt(2, y, 1, cText_, "up %lus %s", (unsigned long)(n / 1000), c.demo ? "DEMO ON" : "real data");
}

void RadarDisplay::scrSettings(const UiContext& c) {
  const Theme& t = th_[c.set->theme];
  header(c, "SETTINGS");
  const UiSettings& s = *c.set;
  char v[kItems][16];
  snprintf(v[0], 16, "%.1f m", rangeMm(s.rangeIdx) / 1000.0f);
  snprintf(v[1], 16, "%d/5", s.brightIdx + 1);
  snprintf(v[2], 16, "%s", th_[s.theme].name);
  snprintf(v[3], 16, "%s", s.alien ? "ALIEN" : "CLASSIC");
  if (kAlertMm[s.alertIdx % 4]) snprintf(v[4], 16, "%.1f m", kAlertMm[s.alertIdx % 4] / 1000.0f); else snprintf(v[4], 16, "OFF");
  snprintf(v[5], 16, "%s", s.trails ? "ON" : "OFF");
  const char* names[kItems] = {"Range", "Bright", "Theme", "Style", "Alert", "Trails"};
  for (int i = 0; i < kItems; i++) {
    int y = 19 + i * 21;
    if (i == s.item) { spr_.fillRect(2, y - 1, W - 4, 19, scale(t.accent, 50)); spr_.drawRect(2, y - 1, W - 4, 19, t.accent); }
    txt(8, y + 1, 2, cText_, "%s", names[i]);
    txt(78, y + 1, 2, t.accent, "%s", v[i]);
  }
  txt(3, 148, 1, cText_, "Demo mode: %s", c.demo ? "ON" : "OFF");
  txt(3, 162, 1, t.dim, "B1 short : next screen");
  txt(3, 172, 1, t.dim, "B1 hold  : next item");
  txt(3, 182, 1, t.dim, "B2 short : change value");
  txt(3, 192, 1, t.dim, "B2 hold  : demo on/off");
  txt(3, 208, 1, t.dim, "Settings are saved.");
}
