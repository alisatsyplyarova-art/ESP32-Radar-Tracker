#include "RadarDisplay.h"
#include <stdarg.h>
#include <math.h>
#include <WiFi.h>

static const uint16_t kRangesMm[7] = {500,1000,1500,2000,3000,4000,6000};
static const uint16_t kAlertMm[7] = {0,1000,2000,3000,4000,5000,6000};
static const float kDeg = 0.017453293f;
static const int W = 160, H = 128;

uint16_t RadarDisplay::rangeMm(uint8_t i) { return kRangesMm[i % 7]; }

bool RadarDisplay::begin() {
  canvas_.begin();
  canvas_.clear(TFT_BLACK);
  canvas_.setTextColor(TFT_WHITE);
  canvas_.drawString("LD-RADAR C3", 8, 8, 2);
  canvas_.drawString("ST7735 160x128", 8, 30, 1);
  canvas_.push();
  Serial.printf("[UI] TFT ST7735, logical %dx%d, CS=%d DC=%d RST=%d SCK=%d MOSI=%d\n",
    W, H, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST, PIN_TFT_SCLK, PIN_TFT_MOSI);
  th_[0] = {canvas_.color565(0,255,100),canvas_.color565(70,150,95),canvas_.color565(20,70,40),"GREEN"};
  th_[1] = {canvas_.color565(255,160,0),canvas_.color565(190,125,35),canvas_.color565(100,65,20),"AMBER"};
  th_[2] = {canvas_.color565(0,210,255),canvas_.color565(0,100,130),canvas_.color565(0,60,90),"CYAN"};
  cBg_=TFT_BLACK; cText_=TFT_WHITE; cWarn_=canvas_.color565(255,190,0);
  cBad_=canvas_.color565(255,60,60); cGood_=canvas_.color565(60,255,120); cGray_=canvas_.color565(120,120,120);
  cSlot_[0]=canvas_.color565(255,145,40); cSlot_[1]=canvas_.color565(190,80,255); cSlot_[2]=canvas_.color565(60,230,100);
  spriteReady_=true;
  canvas_.clear(cBg_);
  canvas_.push();
  return true;
}

void RadarDisplay::setBrightness(uint8_t) {
  // LEDA is tied to the carrier supply; no separate backlight GPIO is connected.
}

uint16_t RadarDisplay::scale(uint16_t c,uint8_t f) const {
  uint16_t r=((c>>11)&31)*f/255, g=((c>>5)&63)*f/255, b=(c&31)*f/255;
  return (r<<11)|(g<<5)|b;
}
void RadarDisplay::txt(int x,int y,uint8_t font,uint16_t col,const char* fmt,...) {
  char b[48]; va_list a; va_start(a,fmt); vsnprintf(b,sizeof(b),fmt,a); va_end(a);
  canvas_.setTextColor(col); spr_.setTextDatum(TL_DATUM); spr_.drawString(b,x,y,font);
}
void RadarDisplay::txtC(int x,int y,uint8_t font,uint16_t col,const char* fmt,...) {
  char b[48]; va_list a; va_start(a,fmt); vsnprintf(b,sizeof(b),fmt,a); va_end(a);
  spr_.setTextColor(col); spr_.setTextDatum(TC_DATUM); spr_.drawString(b,x,y,font);
}
uint16_t RadarDisplay::linkCol(LinkState s) const {
  switch(s) { case LINK_OK:return cGood_; case LINK_LOST:return cWarn_; case LINK_GARBAGE:return cBad_; default:return cGray_; }
}
void RadarDisplay::bar(int x,int y,int w,int h,int val,int maxv,uint16_t col) {
  if(maxv<=0)return; spr_.drawRect(x,y,w,h,scale(col,130));
  int fw=constrain(val,0,maxv)*(w-2)/maxv; if(fw>0)spr_.fillRect(x+1,y+1,fw,h-2,col);
}
bool RadarDisplay::alertActive(const UiContext& c) const {
  if(!c.set||!c.targets)return false;
  uint16_t lim=kAlertMm[c.set->alertIdx%7];
  if(!lim)return false;
  for(uint8_t i=0;i<3;i++) if(c.targets->stateOf(i,c.now)==TS_LIVE) {
    const TrackedTarget& t=c.targets->get(i);
    uint8_t z=t.rawY<0?3:(fabsf((float)t.rawX)<=fabsf((float)t.rawY)*0.5f?0:(t.rawX<0?1:2));
    if((c.set->zoneMask&(1u<<z))&&t.rawDist<lim)return true;
  }
  if(c.presValid&&c.pres.state) {
    uint16_t cm=c.pres.state==2?c.pres.statDistCm:(c.pres.movDistCm?c.pres.movDistCm:c.pres.statDistCm);
    if(cm&&(uint32_t)cm*10<lim)return true;
  }
  return false;
}

void RadarDisplay::render(const UiContext& c) {
  if(!spriteReady_||!c.set||!c.targets||!c.ld2450||!c.ld2401)return;
  const Theme& th=th_[c.set->theme%3];
  spr_.fillSprite(cBg_);
  const LinkState l2450=c.ld2450->link(c.now), l2401=c.ld2401->link(c.now);
  const bool alert=alertActive(c);
  spr_.fillRect(0,0,W,15,alert?scale(cBad_,100):scale(th.accent,45));
  txt(3,1,1,alert?cBad_:th.accent,"%s",alert?"! ALERT":(c.demo?"LD-RADAR C3 DEMO":"LD-RADAR C3"));
  spr_.fillCircle(143,7,3,linkCol(l2450==LINK_OK?LINK_OK:(l2401==LINK_OK?LINK_NO_DATA:l2450)));
  spr_.fillCircle(153,7,3,linkCol(l2401==LINK_OK?LINK_OK:(l2450==LINK_OK?LINK_NO_DATA:l2401)));

  if(c.screen==SCR_RADAR && l2450==LINK_OK) {
    const int ox=80, oy=120, R=92;
    const uint32_t rng=rangeMm(c.set->rangeIdx); const float k=(float)R/rng;
    for(int ring=1;ring<=5;ring++) {
      int rr=R*ring/5, px=ox-rr, py=oy;
      for(int a=180;a>=0;a-=3) {
        float q=a*kDeg; int nx=ox+(int)(rr*cosf(q)), ny=oy-(int)(rr*sinf(q));
        spr_.drawLine(px,py,nx,ny,scale(th.accent,ring==5?180:85)); px=nx;py=ny;
      }
      if(rr>12) txt(ox+3,oy-rr-7,1,th.dim,"%.1fm",(float)rng*ring/5000.0f);
    }
    spr_.drawLine(ox-R,oy,ox+R,oy,th.accent);
    spr_.drawLine(ox,oy-R,ox,oy,scale(th.accent,160));
    if(c.set->sweep) {
      float phase=(c.now%SWEEP_PERIOD_MS)/(float)SWEEP_PERIOD_MS;
      float a=(-60.0f+120.0f*phase)*kDeg;
      int bx=ox+(int)(R*sinf(a)), by=oy-(int)(R*cosf(a));
      spr_.drawLine(ox,oy,bx,by,th.accent);
    }
    for(uint8_t i=0;i<3;i++) {
      const TrackedTarget& t=c.targets->get(i); TargetState st=c.targets->stateOf(i,c.now);
      if(st==TS_NONE&&(!c.set->trails||t.trailCount<2))continue;
      uint16_t col=cSlot_[i];
      if(c.set->trails&&t.trailCount>1) {
        for(int j=(int)t.trailCount-1;j>=1;j--) {
          TrailPoint a,b;
          if(!RadarTargets::trailAt(t,(uint16_t)j,a)||!RadarTargets::trailAt(t,(uint16_t)(j-1),b))continue;
          uint32_t age=(RadarTargets::trailAgeMs(c.now,a)+RadarTargets::trailAgeMs(c.now,b))/2;
          if(age>(uint32_t)c.set->trailSeconds*1000UL)continue;
          int x1=ox+(int)(a.x*k),y1=oy-(int)(a.y*k),x2=ox+(int)(b.x*k),y2=oy-(int)(b.y*k);
          if(x1<1||x1>=W-1||x2<1||x2>=W-1||y1<17||y1>oy||y2<17||y2>oy)continue;
          uint16_t faded=scale(col,(uint8_t)constrain(170-(int)(age*90UL/((uint32_t)c.set->trailSeconds*1000UL)),70,170));
          spr_.drawLine(x1,y1,x2,y2,faded);
        }
      }
      if(st==TS_NONE)continue;
      int x=ox+(int)(t.rawX*k), y=oy-(int)(t.rawY*k);
      if(x<3||x>W-3||y<18||y>oy)continue;
      if(st==TS_HOLD)spr_.drawCircle(x,y,4,scale(col,120));
      else {spr_.fillCircle(x,y,3,col);spr_.drawCircle(x,y,5,TFT_WHITE);}
      txt(x+5,y-7,1,col,"T%u",i+1);
    }
    txt(3,17,1,th.dim,"LD2450 / X-Y / %.1fm",(float)rng/1000.0f);
    if(c.targets->liveCount(c.now)==0)txtC(80,61,1,th.dim,"NO TARGETS");
  } else if(c.screen==SCR_RADAR || c.screen==SCR_PRESENCE || l2401==LINK_OK) {
    // LD2401 is a one-dimensional presence sensor; show distance on a fan, not fake X/Y.
    const int ox=80,oy=118,R=88; const uint16_t rng=rangeMm(c.set->rangeIdx);
    const LD2401Data& d=c.pres;
    static int16_t hist[32]; static uint8_t head=0,count=0; static uint32_t last=0;
    if(c.presValid&&c.now-last>=500) {
      last=c.now; uint16_t cm=d.state==2?d.statDistCm:(d.movDistCm?d.movDistCm:d.statDistCm);
      hist[head]=(!d.state||!cm)?-1:(int16_t)cm; head=(head+1)%32; if(count<32)count++;
    }
    for(int ring=1;ring<=4;ring++) {
      int rr=R*ring/4, px=ox+(int)(rr*sinf(-60*kDeg)), py=oy-(int)(rr*cosf(-60*kDeg));
      for(int a=-57;a<=60;a+=3) {
        int nx=ox+(int)(rr*sinf(a*kDeg)),ny=oy-(int)(rr*cosf(a*kDeg));
        spr_.drawLine(px,py,nx,ny,scale(th.accent,ring==4?180:85));px=nx;py=ny;
      }
    }
    spr_.drawLine(ox,oy,ox+(int)(R*sinf(-60*kDeg)),oy-(int)(R*cosf(-60*kDeg)),th.accent);
    spr_.drawLine(ox,oy,ox+(int)(R*sinf(60*kDeg)),oy-(int)(R*cosf(60*kDeg)),th.accent);
    int maxCm=rng/10;
    for(uint8_t i=0;i<count;i++) {
      int16_t cm=hist[(head+32-count+i)%32]; if(cm<=0||cm>maxCm)continue;
      int rr=4+(int)cm*(R-4)/maxCm;
      spr_.fillCircle(ox,oy-rr,i==count-1?3:1,i==count-1?cSlot_[0]:scale(cSlot_[0],100));
    }
    uint16_t cm=d.state==2?d.statDistCm:(d.movDistCm?d.movDistCm:d.statDistCm);
    if(c.presValid&&cm>0&&cm<=maxCm) {
      int yy=oy-(4+(int)cm*(R-4)/maxCm);
      spr_.drawFastHLine(ox-6,yy,13,cSlot_[0]);spr_.fillCircle(ox,yy,3,cSlot_[0]);
    }
    txt(3,17,1,th.dim,"LD2401 / PRESENCE");
    txt(3,108,1,c.presValid?(d.state?cWarn_:cGood_):cBad_,"%s",c.presValid?RadarLD2401::stateStr(d.state):"NO DATA");
    txt(100,108,1,cText_,"%ucm",c.presValid?cm:0);
  } else if(c.screen==SCR_TARGETS) {
    txt(4,19,1,th.dim,"LD2450 TARGETS");
    for(uint8_t i=0;i<3;i++) {
      const TrackedTarget& t=c.targets->get(i); TargetState st=c.targets->stateOf(i,c.now);
      int y=37+i*27; txt(4,y,2,cSlot_[i],"T%u",i+1);
      if(st==TS_NONE)txt(40,y+2,1,th.dim,"NO TARGET");
      else txt(40,y+2,1,cText_,"X%+.1f Y%.1f D%.1fm",t.rawX/1000.0f,t.rawY/1000.0f,t.rawDist/1000.0f);
    }
  } else if(c.screen==SCR_DUAL) {
    txt(4,22,1,linkCol(l2450),"LD2450 %s",linkStateStr(l2450));
    txt(4,43,1,linkCol(l2401),"LD2401 %s",linkStateStr(l2401));
    txt(4,66,1,cText_,"Targets %u/3",c.targets->liveCount(c.now));
    if(c.presValid)txt(4,88,1,cWarn_,"Presence %ucm",c.pres.detDistCm);
    txt(4,111,1,th.dim,"Connect one sensor at a time");
  } else if(c.screen==SCR_DIAG) {
    txt(4,20,1,th.accent,"DIAGNOSTICS");
    txt(4,36,1,linkCol(l2450),"LD2450 %s",linkStateStr(l2450));
    txt(4,50,1,cText_,"RX %lu bad %lu",(unsigned long)c.ld2450->stats().bytesRx,(unsigned long)c.ld2450->stats().framesBad);
    txt(4,68,1,linkCol(l2401),"LD2401 %s",linkStateStr(l2401));
    txt(4,82,1,cText_,"RX %lu bad %lu",(unsigned long)c.ld2401->stats().bytesRx,(unsigned long)c.ld2401->stats().framesBad);
    txt(4,101,1,th.dim,"Heap %uK FPS %.0f",(unsigned)(ESP.getFreeHeap()/1024),c.uiFps);
  } else if(c.screen==SCR_SETTINGS) {
    const char* names[6]={"Range","Brightness","Theme","Alert","Trails","Trail time"};
    char vals[6][12];
    snprintf(vals[0],12,"%.1fm",rangeMm(c.set->rangeIdx)/1000.0f);
    snprintf(vals[1],12,"%u/5",c.set->brightIdx+1);
    snprintf(vals[2],12,"%s",th_[c.set->theme%3].name);
    snprintf(vals[3],12,"%s",c.set->alertIdx?"ON":"OFF");
    snprintf(vals[4],12,"%s",c.set->trails?"ON":"OFF");
    snprintf(vals[5],12,"%us",c.set->trailSeconds);
    for(int i=0;i<6;i++){int y=20+i*16;if(i==c.set->item)spr_.fillRect(2,y-1,156,14,scale(th.accent,50));txt(5,y,1,cText_,"%s",names[i]);txt(112,y,1,th.accent,"%s",vals[i]);}
    txt(4,118,1,th.dim,"Use web page to change");
  } else if(c.screen==SCR_WIFI) {
    bool connected=WiFi.status()==WL_CONNECTED;
    txt(4,20,1,connected?cGood_:cWarn_,"HOME WI-FI %s",connected?"OK":"OFF");
    String ssid=connected?WiFi.SSID():String("NOT CONNECTED");
    txt(4,39,1,th.dim,"SSID: %s",ssid.c_str());
    txt(4,59,1,cText_,"IP: %s",connected?WiFi.localIP().toString().c_str():"--");
    txt(4,81,1,th.accent,"AP: %s",WiFi.softAPIP().toString().c_str());
  }
  if(c.demo)txt(4,119,1,cWarn_,"DEMO");
  spr_.pushSprite(0,0);
}
