#include "RadarLD2450.h"
#include <math.h>

static const uint8_t kHdr[4] = {0xAA, 0xFF, 0x03, 0x00};

void RadarLD2450::beginShared(HardwareSerial& s, uint32_t baud) {
  ser_ = &s; rx_ = -1; tx_ = -1; baud_ = baud;
  idx_ = 0; link_.reset(millis());
#if LD2450_SEND_MULTI_TARGET_CMD
  cfgStep_ = 1; cfgNext_ = millis() + 500;
#endif
}

void RadarLD2450::begin(HardwareSerial& s, int rxPin, int txPin, uint32_t baud) {
  ser_ = &s; rx_ = rxPin; tx_ = txPin; baud_ = baud;
  openPort();
  link_.reset(millis());
#if LD2450_SEND_MULTI_TARGET_CMD
  cfgStep_ = 1; cfgNext_ = millis() + 500;
#endif
}

void RadarLD2450::openPort() {
  ser_->end();
  ser_->setRxBufferSize(1024);
  ser_->begin(baud_, SERIAL_8N1, rx_, tx_);
  idx_ = 0;
}

// Command frame: FD FC FB FA | len(2) | cmd(2) | value | 04 03 02 01
void RadarLD2450::sendCmd(uint16_t cmd, const uint8_t* val, uint8_t vlen) {
  uint16_t len = 2 + vlen;
  const uint8_t head[] = {0xFD, 0xFC, 0xFB, 0xFA, (uint8_t)(len & 0xFF), (uint8_t)(len >> 8),
                          (uint8_t)(cmd & 0xFF), (uint8_t)(cmd >> 8)};
  const uint8_t tail[] = {0x04, 0x03, 0x02, 0x01};
  ser_->write(head, sizeof(head));
  if (vlen) ser_->write(val, vlen);
  ser_->write(tail, sizeof(tail));
}

void RadarLD2450::update(uint32_t now) {
  if (!ser_) return;
  int n = 0;
  while (ser_->available() && n < 768) { feedByte((uint8_t)ser_->read(), now); n++; }

  runConfig(now);

  link_.tick(now);
  if (link_.needRestart(now)) {           // automatic recovery from UART problems
    openPort();
    link_.markRestart(now);
#if LD2450_SEND_MULTI_TARGET_CMD
    cfgStep_ = 1; cfgNext_ = now + 300;
#endif
    Serial.println(F("[LD2450] no valid frames -> UART restarted"));
  }
}

// Non-blocking config sequence: enable config -> multi-target mode -> end config.
// Shared between the standalone path (update) and the shared-UART path (tickShared).
void RadarLD2450::runConfig(uint32_t now) {
  if (!cfgStep_ || (int32_t)(now - cfgNext_) < 0) return;
  static const uint8_t en[2] = {0x01, 0x00};
  if (cfgStep_ == 1)      sendCmd(0x00FF, en, 2);
  else if (cfgStep_ == 2) sendCmd(0x0090, nullptr, 0);
  else                    sendCmd(0x00FE, nullptr, 0);
  cfgStep_ = (cfgStep_ >= 3) ? 0 : cfgStep_ + 1;
  cfgNext_ = now + 80;
}

// Shared mode: the UART belongs to main.cpp, so no port restart here. A restart
// would reset the UART that the LD2401 parser is also using.
void RadarLD2450::tickShared(uint32_t now) {
  runConfig(now);
  link_.tick(now);
}

void RadarLD2450::sharedPortRestarted(uint32_t now) {
  idx_ = 0; link_.markRestart(now);
#if LD2450_SEND_MULTI_TARGET_CMD
  cfgStep_ = 1; cfgNext_ = now + 300;
#endif
}

void RadarLD2450::feedByte(uint8_t b, uint32_t now) {
  link_.bytesRx++;
  if (idx_ < 4) {
    if (b == kHdr[idx_]) buf_[idx_++] = b;
    else { idx_ = 0; if (b == kHdr[0]) buf_[idx_++] = b; }
    return;
  }
  buf_[idx_++] = b;
  if (idx_ == kFrameLen) { idx_ = 0; parseFrame(now); }
}

void RadarLD2450::parseFrame(uint32_t now) {
  if (buf_[28] != 0x55 || buf_[29] != 0xCC) { link_.framesBad++; return; }
  LD2450Target t[3];
  for (int i = 0; i < 3; i++) {
    const uint8_t* p = &buf_[4 + 8 * i];
    uint16_t rx = p[0] | (p[1] << 8), ry = p[2] | (p[3] << 8);
    uint16_t rs = p[4] | (p[5] << 8), rr = p[6] | (p[7] << 8);
    if (!rx && !ry && !rs && !rr) continue;                    // empty slot
    int16_t x = decodeSigned(rx), y = decodeSigned(ry);
    if (abs(x) > LD2450_MAX_ABS_X || y < 0 || y > LD2450_MAX_Y) { rejected_++; continue; }
    t[i].valid = true; t[i].x = x; t[i].y = y;
    t[i].speed = decodeSigned(rs); t[i].res = rr;
    t[i].dist = sqrtf((float)x * x + (float)y * y);
  }
  for (int i = 0; i < 3; i++) pending_[i] = t[i];
  frameNew_ = true;
  link_.frameOk(now);
}

bool RadarLD2450::consumeFrame(LD2450Target out[3]) {
  if (!frameNew_) return false;
  for (int i = 0; i < 3; i++) out[i] = pending_[i];
  frameNew_ = false;
  return true;
}
