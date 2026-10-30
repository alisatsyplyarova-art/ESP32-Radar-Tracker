#pragma once
#include <Arduino.h>
#include "LinkMonitor.h"
struct LD2401Data { uint8_t state=0; uint16_t movDistCm=0; uint8_t movEnergy=0; uint16_t statDistCm=0; uint8_t statEnergy=0; uint16_t detDistCm=0; };
class RadarLD2401 {
public:
  void begin(HardwareSerial& s,int rxPin,int txPin,uint32_t baud); void update(uint32_t now); void feedByte(uint8_t b,uint32_t now);
  LinkState link(uint32_t now) const { return link_.state(now); } const LinkMonitor& stats() const { return link_; }
  const LD2401Data& data() const { return data_; } uint32_t baud() const { return baud_; }
  static const char* stateStr(uint8_t st);
private:
  void openPort(); void parsePayload(const uint8_t* p,uint16_t len,uint32_t now);
  HardwareSerial* ser_=nullptr; int rx_=-1,tx_=-1; uint32_t baud_=256000; LinkMonitor link_; LD2401Data data_;
  enum { S_HDR,S_LEN,S_BODY,S_TAIL } st_=S_HDR;
  uint8_t hdrIdx_=0,tailIdx_=0,lenIdx_=0; uint16_t len_=0,bodyIdx_=0; uint8_t body_[64]={};
};