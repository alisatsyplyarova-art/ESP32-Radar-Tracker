#pragma once
#include <Arduino.h>
#include "LinkMonitor.h"
struct LD2450Target { bool valid=false; int16_t x=0,y=0,speed=0; uint16_t res=0; float dist=0; };
class RadarLD2450 {
public:
  static const uint8_t kFrameLen=30;
  void begin(HardwareSerial& s,int rxPin,int txPin,uint32_t baud);
  void update(uint32_t now); void feedByte(uint8_t b,uint32_t now);
  bool consumeFrame(LD2450Target out[3]);
  LinkState link(uint32_t now) const { return link_.state(now); }
  const LinkMonitor& stats() const { return link_; }
  uint32_t rejected() const { return rejected_; } uint32_t baud() const { return baud_; }
  static int16_t decodeSigned(uint16_t raw) { int16_t m=(int16_t)(raw&0x7FFF); return (raw&0x8000)?m:(int16_t)-m; }
private:
  void openPort(); void parseFrame(uint32_t now); void sendCmd(uint16_t cmd,const uint8_t* val,uint8_t vlen);
  HardwareSerial* ser_=nullptr; int rx_=-1,tx_=-1; uint32_t baud_=256000;
  uint8_t buf_[kFrameLen]; uint8_t idx_=0; LinkMonitor link_; LD2450Target pending_[3]; bool frameNew_=false;
  uint32_t rejected_=0; uint8_t cfgStep_=0; uint32_t cfgNext_=0;
};