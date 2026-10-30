#include "RadarLD2401.h"
static const uint8_t kHdr[4]={0xF4,0xF3,0xF2,0xF1};static const uint8_t kTail[4]={0xF8,0xF7,0xF6,0xF5};
void RadarLD2401::begin(HardwareSerial& s,int rxPin,int txPin,uint32_t baud){ser_=&s;rx_=rxPin;tx_=txPin;baud_=baud;openPort();link_.reset(millis());}
void RadarLD2401::openPort(){if(!ser_)return;ser_->end();ser_->setRxBufferSize(1024);ser_->begin(baud_,SERIAL_8N1,rx_,tx_);st_=S_HDR;hdrIdx_=tailIdx_=lenIdx_=0;len_=bodyIdx_=0;}
const char* RadarLD2401::stateStr(uint8_t st){switch(st){case 1:return "MOVING";case 2:return "STATIC";case 3:return "BOTH";default:return "CLEAR";}}
void RadarLD2401::update(uint32_t now){if(!ser_)return;int n=0;while(ser_->available()&&n<768){feedByte((uint8_t)ser_->read(),now);++n;}link_.tick(now);if(link_.needRestart(now)){openPort();link_.markRestart(now);Serial.println(F("[LD2401] no valid frames -> UART restarted"));}}
void RadarLD2401::feedByte(uint8_t b,uint32_t now){link_.bytesRx++;switch(st_){
case S_HDR:if(b==kHdr[hdrIdx_]){if(++hdrIdx_==4){st_=S_LEN;lenIdx_=0;len_=0;}}else hdrIdx_=(b==kHdr[0])?1:0;break;
case S_LEN:if(lenIdx_==0){len_=b;lenIdx_=1;}else{len_|=(uint16_t)b<<8;if(len_<13||len_>sizeof(body_)){link_.framesBad++;st_=S_HDR;hdrIdx_=0;}else{st_=S_BODY;bodyIdx_=0;}}break;
case S_BODY:body_[bodyIdx_++]=b;if(bodyIdx_==len_){st_=S_TAIL;tailIdx_=0;}break;
case S_TAIL:if(b!=kTail[tailIdx_]){link_.framesBad++;st_=S_HDR;hdrIdx_=(b==kHdr[0])?1:0;break;}if(++tailIdx_==4){parsePayload(body_,len_,now);st_=S_HDR;hdrIdx_=0;}break;}}
void RadarLD2401::parsePayload(const uint8_t* p,uint16_t len,uint32_t now){if(len<13||p[0]!=0x02||p[1]!=0xAA||p[len-2]!=0x55||p[len-1]!=0x00){link_.framesBad++;return;}LD2401Data d;d.state=p[2]&0x03;d.movDistCm=(uint16_t)p[3]|((uint16_t)p[4]<<8);d.movEnergy=p[5];d.statDistCm=(uint16_t)p[6]|((uint16_t)p[7]<<8);d.statEnergy=p[8];d.detDistCm=(uint16_t)p[9]|((uint16_t)p[10]<<8);data_=d;link_.frameOk(now);}