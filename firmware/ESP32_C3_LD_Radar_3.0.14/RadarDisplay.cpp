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
  canvas_.setTextColor(col); canvas_.setTextDatum(TL_DATUM); canvas_.drawString(b,x,y,font);
}
void RadarDisplay::txtC(int x,int y,uint8_t font,uint16_t col,const char* fmt,...) {
  char b[48]; va_list a; va_start(a,fmt); vsnprintf(b,sizeof(b),fmt,a); va_end(a);
  canvas_.setTextColor(col); canvas_.setTextDatum(TC_DATUM); canvas_.drawString(b,x,y,font);
}
uint16_t RadarDisplay::linkCol(LinkState s) const {
  switch(s) { case LINK_OK:return cGood_; case LINK_LOST:return cWarn_; case LINK_GARBAGE:return cBad_; default:return cGray_; }
}
void RadarDisplay::bar(int x,int y,int w,int h,int val,int maxv,uint16_t col) {
  if(maxv<=0)return; canvas_.drawRect(x,y,w,h,scale(col,130));
  int fw=constrain(val,0,maxv)*(w-2)/maxv; if(fw>0)canvas_.fillRect(x+1,y+1,fw,h-2,col);
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
  canvas_.clear(cBg_);
  const LinkState l2450=c.ld2450->link(c.now), l2401=c.ld2401->link(c.now);
  const bool alert=alertActive(c);
  canvas_.fillRect(0,0,W,15,alert?scale(cBad_,100):scale(th.accent,45));
  txt(3,1,1,alert?cBad_:th.accent,"%s",alert?"! ALERT":(c.demo?"LD-RADAR C3 DEMO":"LD-RADAR C3"));
  canvas_.fillCircle(143,7,3,linkCol(l2450==LINK_OK?LINK_OK:(l2401==LINK_OK?LINK_NO_DATA:l2450)));
  canvas_.fillCircle(153,7,3,linkCol(l2401==LINK_OK?LINK_OK:(l2450==LINK_OK?LINK_NO_DATA:l2401)));

  if(c.screen==SCR_RADAR && l2450==LINK_OK) {
    const int ox=80, oy=120, R=92;
    const uint32_t rng=rangeMm(c.set->rangeIdx); const float k=(float)R/rng;
    for(int ring=1;ring<=5;ring++) {
      int rr=R*ring/5;
      canvas_.drawArcMidpoint(ox,oy,rr,0.0f,180.0f,
                              scale(th.accent,ring==5?190:95),false);
      if(rr>12) txt(ox+3,oy-rr-7,1,th.dim,"%.1fm",(float)rng*ring/5000.0f);
    }
    canvas_.drawLine(ox-R,oy,ox+R,oy,th.accent);
    canvas_.drawLine(ox,oy-R,ox,oy,scale(th.accent,160));
    if(c.set->sweep) {
      float phase=(c.now%SWEEP_PERIOD_MS)/(float)SWEEP_PERIOD_MS;
      float a=(-60.0f+120.0f*phase)*kDeg;
      int bx=ox+(int)(R*sinf(a)), by=oy-(int)(R*cosf(a));
      canvas_.drawLine(ox,oy,bx,by,th.accent);
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
          canvas_.drawLine(x1,y1,x2,y2,faded);
        }
      }
      if(st==TS_NONE)continue;
      int x=ox+(int)(t.rawX*k), y=oy-(int)(t.rawY*k);
      if(x<3||x>W-3||y<18||y>oy)continue;
      if(st==TS_HOLD)canvas_.drawCircle(x,y,4,scale(col,120));
      else {canvas_.fillCircle(x,y,3,col);canvas_.drawCircle(x,y,5,TFT_WHITE);}
      txt(x+5,y-7,1,col,"T%u",i+1);
    }
    txt(3,17,1,th.dim,"LD2450 / X-Y / %.1fm",(float)rng/1000.0f);
    if(c.targets->liveCount(c.now)==0)txtC(80,61,1,th.dim,"NO TARGETS");
  } else if(c.screen==SCR_RADAR || c.screen==SCR_PRESENCE || l2401==LINK_OK) {
    // LD2401: dedicated one-dimensional presence display, not an X/Y target plot.
    const int ox=80,oy=119,R=82; const uint16_t rng=rangeMm(c.set->rangeIdx);
    const LD2401Data& d=c.pres;
    static int16_t hist[24]; static uint8_t head=0,count=0; static uint32_t last=0;
    uint16_t cm=d.state==2?d.statDistCm:(d.movDistCm?d.movDistCm:d.statDistCm);
    uint8_t energy=d.state==2?d.statEnergy:d.movEnergy;
    if(c.presValid&&c.now-last>=350) {
      last=c.now; hist[head]=(!d.state||!cm)?-1:(int16_t)cm; head=(head+1)%24; if(count<24)count++;
    }
    // Header strip and compact telemetry.
    canvas_.fillRect(0,15,W,14,scale(th.accent,35));
    txt(4,17,1,th.accent,"LD2401  /  PRESENCE");
    txt(119,17,1,linkCol(l2401),"%s",c.presValid?"ONLINE":"OFFLINE");
    // Radar fan with four range rings and three bearing guides.
    for(int ring=1;ring<=4;ring++) {
      int rr=R*ring/4;
      canvas_.drawArcMidpoint(ox,oy,rr,-57.0f,60.0f,
                              scale(th.accent,ring==4?195:85),true);
      if(ring==2||ring==4)txt(ox+4,oy-rr-8,1,th.dim,"%.1fm",(float)rng*ring/4000.0f);
    }
    for(int a=-60;a<=60;a+=30) {
      int ex=ox+(int)(R*sinf(a*kDeg)),ey=oy-(int)(R*cosf(a*kDeg));
      canvas_.drawLine(ox,oy,ex,ey,scale(th.accent,a==0?150:65));
    }
    // Animated sweep for LD2401 presence fan.
    if(c.set->sweep) {
      const float phase=(c.now%SWEEP_PERIOD_MS)/(float)SWEEP_PERIOD_MS;
      const float a=(-60.0f+120.0f*phase)*kDeg;
      const int bx=ox+(int)(R*sinf(a)), by=oy-(int)(R*cosf(a));
      canvas_.drawLine(ox,oy,bx,by,th.accent);
    }
    // Historical detections are dim; the latest valid return is bright orange.
    for(uint8_t i=0;i<count;i++) {
      int16_t hcm=hist[(head+24-count+i)%24]; if(hcm<=0||hcm>rng/10)continue;
      int rr=4+(int)hcm*(R-4)/(rng/10);
      canvas_.fillCircle(ox,oy-rr,1,scale(cSlot_[0],85));
    }
    if(c.presValid&&d.state&&cm>0&&cm<=rng/10) {
      int yy=oy-(4+(int)cm*(R-4)/(rng/10));
      canvas_.drawFastHLine(ox-9,yy,19,cSlot_[0]);canvas_.fillCircle(ox,yy,4,cSlot_[0]);canvas_.drawCircle(ox,yy,6,TFT_WHITE);
    }
    canvas_.fillCircle(ox,oy,4,cText_);
    // Footer: state, measured range and signal energy.
    const char* state=!c.presValid?"NO LINK":d.state==1?"MOVING":d.state==2?"STATIC":d.state==3?"MOV + STATIC":"CLEAR";
    txt(3,103,1,c.presValid?(d.state?cWarn_:cGood_):cBad_,"%s",state);
    if(c.presValid&&d.state&&cm>0)txt(95,101,2,cSlot_[0],"%.2fm",cm/100.0f);
    else txt(104,103,1,th.dim,"--.--m");
    txt(3,116,1,th.dim,"RANGE %.1fm",rng/1000.0f);
    bar(105,117,49,7,c.presValid?energy:0,100,cSlot_[0]);
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
    for(int i=0;i<6;i++){int y=20+i*16;if(i==c.set->item)canvas_.fillRect(2,y-1,156,14,scale(th.accent,50));txt(5,y,1,cText_,"%s",names[i]);txt(112,y,1,th.accent,"%s",vals[i]);}
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
  canvas_.push();
}
