#pragma once
#include <Arduino.h>
#include "RadarCanvas.h"
#include "RadarTargets.h"
#include "RadarLD2450.h"
#include "RadarLD2401.h"
#define TFT_BLACK 0x0000
#define TFT_WHITE 0xFFFF
#define TL_DATUM 0
#define TC_DATUM 1
enum Screen : uint8_t { SCR_RADAR, SCR_TARGETS, SCR_PRESENCE, SCR_DUAL, SCR_DIAG, SCR_SETTINGS, SCR_WIFI, SCR_COUNT };
struct UiSettings { uint8_t rangeIdx=6,brightIdx=3,theme=0; bool trails=true; uint16_t trailSeconds=5; bool sweep=false; uint8_t alertIdx=0,zoneMask=0x01,item=0; };
struct UiContext { Screen screen=SCR_RADAR; bool demo=false; uint32_t now=0; const RadarTargets* targets=nullptr; const RadarLD2450* ld2450=nullptr; const RadarLD2401* ld2401=nullptr; bool presValid=false; LD2401Data pres; float uiFps=0; const UiSettings* set=nullptr; };
class RadarDisplay {
public:
 bool begin(); void setBrightness(uint8_t idx); void render(const UiContext& c);
 const uint16_t* framebuffer(){return spriteReady_?canvas_.getPointer():nullptr;}
 static constexpr uint16_t kWidth=160,kHeight=128;
 static uint16_t rangeMm(uint8_t idx);
 static const uint8_t kRanges=7,kBright=5,kThemes=3,kAlerts=7,kItems=6;
private:
 struct Theme{uint16_t accent,dim,grid;const char* name;};
 RadarCanvas canvas_; bool spriteReady_=false; Theme th_[3];
 uint16_t cBg_=0,cText_=0,cWarn_=0,cBad_=0,cGood_=0,cGray_=0,cSlot_[3]={};
 uint16_t scale(uint16_t c,uint8_t f)const;
 void txt(int x,int y,uint8_t font,uint16_t col,const char* fmt,...);
 void txtC(int x,int y,uint8_t font,uint16_t col,const char* fmt,...);
 uint16_t linkCol(LinkState s)const; void bar(int x,int y,int w,int h,int val,int maxv,uint16_t col);
 bool alertActive(const UiContext& c)const;
};
