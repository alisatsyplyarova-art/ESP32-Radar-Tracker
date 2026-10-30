#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "RadarTargets.h"
#include "RadarLD2450.h"
#include "RadarLD2401.h"
enum Screen:uint8_t { SCR_RADAR,SCR_TARGETS,SCR_PRESENCE,SCR_DUAL,SCR_DIAG,SCR_SETTINGS,SCR_COUNT };
struct UiSettings { uint8_t rangeIdx=6,brightIdx=3,theme=0; bool trails=true,alien=true; uint8_t alertIdx=0,item=0; };
struct UiContext {
 Screen screen=SCR_RADAR; bool demo=false; uint32_t now=0; const RadarTargets* targets=nullptr;
 const RadarLD2450* ld2450=nullptr; const RadarLD2401* ld2401=nullptr; bool presValid=false; LD2401Data pres; float uiFps=0; const UiSettings* set=nullptr;
};
class RadarDisplay {
public:
 bool begin(); void setBrightness(uint8_t idx); void render(const UiContext& c); static uint16_t rangeMm(uint8_t idx);
 RadarDisplay():spr_(&tft_){} static const uint8_t kRanges=7,kBright=5,kThemes=3,kAlerts=4,kItems=6;
private:
 struct Theme { uint16_t accent,dim,grid; const char* name; };
 TFT_eSPI tft_; TFT_eSprite spr_; Theme th_[3]; uint16_t cBg_,cText_,cWarn_,cBad_,cGood_,cGray_,cSlot_[3];
 uint16_t scale(uint16_t c,uint8_t f) const; void txt(int x,int y,uint8_t font,uint16_t col,const char* fmt,...);
 void txtC(int x,int y,uint8_t font,uint16_t col,const char* fmt,...); void header(const UiContext& c,const char* title);
 void footer(const UiContext& c); void field(const UiContext& c,int ox,int oy,int R); void bar(int x,int y,int w,int h,int val,int maxv,uint16_t col);
 uint16_t linkCol(LinkState s) const; bool alertActive(const UiContext& c) const;
 void scrRadar(const UiContext& c); void scrTargets(const UiContext& c); void scrPresence(const UiContext& c);
 void scrDual(const UiContext& c); void scrDiag(const UiContext& c); void scrSettings(const UiContext& c);
};