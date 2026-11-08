#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <math.h>
#include <string.h>
#include "Config.h"

// Small framebuffer renderer for ST7735. No TFT_eSPI dependency.
class RadarCanvas {
public:
  static constexpr int16_t W=160, H=128;
  bool begin() {
    pinMode(PIN_TFT_CS,OUTPUT); pinMode(PIN_TFT_DC,OUTPUT); pinMode(PIN_TFT_RST,OUTPUT); digitalWrite(PIN_TFT_CS,HIGH); digitalWrite(PIN_TFT_DC,HIGH); digitalWrite(PIN_TFT_RST,HIGH); delay(5);
    digitalWrite(PIN_TFT_RST,LOW); delay(20); digitalWrite(PIN_TFT_RST,HIGH); delay(120);
    SPI.begin(PIN_TFT_SCLK,-1,PIN_TFT_MOSI,PIN_TFT_CS);
    SPI.beginTransaction(SPISettings(10000000,MSBFIRST,SPI_MODE0));
    command(0x01); delay(150);
    command(0x11); delay(120);
    commandData(0x3A,0x05);
    commandData(0x36,0x60);
    command(0x29); delay(20);
    SPI.endTransaction();
    pinMode(PIN_TFT_CS,OUTPUT); digitalWrite(PIN_TFT_CS,HIGH);
    return true;
  }
  uint16_t color565(uint8_t r,uint8_t g,uint8_t b) const { return (uint16_t(r&0xF8)<<8)|(uint16_t(g&0xFC)<<3)|(b>>3); }
  void clear(uint16_t c){ for(int i=0;i<W*H;i++) pixels_[i]=c; }
  void fillRect(int x,int y,int w,int h,uint16_t c){
    int x0=(x<0?0:x),y0=(y<0?0:y),x1=((x+w)>W?W:(x+w)),y1=((y+h)>H?H:(y+h));
    for(int yy=y0;yy<y1;yy++) for(int xx=x0;xx<x1;xx++) pixels_[yy*W+xx]=c;
  }
  void drawRect(int x,int y,int w,int h,uint16_t c){ drawFastHLine(x,y,w,c);drawFastHLine(x,y+h-1,w,c);drawFastVLine(x,y,h,c);drawFastVLine(x+w-1,y,h,c); }
  void drawFastHLine(int x,int y,int w,uint16_t c){ if(y<0||y>=H)return; int a=(x<0?0:x),b=((x+w)>W?W:(x+w));for(int i=a;i<b;i++)pixels_[y*W+i]=c; }
  void drawFastVLine(int x,int y,int h,uint16_t c){ if(x<0||x>=W)return;int a=(y<0?0:y),b=((y+h)>H?H:(y+h));for(int i=a;i<b;i++)pixels_[i*W+x]=c; }
  void drawPixel(int x,int y,uint16_t c){if(x>=0&&x<W&&y>=0&&y<H)pixels_[y*W+x]=c;}
  void drawLine(int x0,int y0,int x1,int y1,uint16_t c){
    int dx=abs(x1-x0),sx=x0<x1?1:-1,dy=-abs(y1-y0),sy=y0<y1?1:-1,err=dx+dy;
    for(;;){drawPixel(x0,y0,c);if(x0==x1&&y0==y1)break;int e=2*err;if(e>=dy){err+=dy;x0+=sx;}if(e<=dx){err+=dx;y0+=sy;}}
  }

  // Midpoint/Bresenham circle arc, ported from the working ST7735 radar test.
  // Eight-way symmetry keeps the curve uniform and avoids trig/segment joins.
  // verticalAxis=false: angles 0..180 form the upper semicircle.
  // verticalAxis=true: angles are measured from the vertical axis (LD2401 fan).
  void drawArcMidpoint(int cx,int cy,int radius,float startDeg,float endDeg,
                       uint16_t color,bool verticalAxis=false) {
    if(radius<=0)return;
    int x=radius,y=0,err=1-radius;
    auto plot=[&](int px,int py) {
      const float dx=(float)(px-cx), dy=(float)(cy-py);
      const float angle=verticalAxis
        ? atan2f(dx,dy)*57.2957795f
        : atan2f(dy,dx)*57.2957795f;
      if(angle>=startDeg && angle<=endDeg) drawPixel(px,py,color);
    };
    while(x>=y) {
      plot(cx+x,cy+y); plot(cx+y,cy+x);
      plot(cx-y,cy+x); plot(cx-x,cy+y);
      plot(cx-x,cy-y); plot(cx-y,cy-x);
      plot(cx+y,cy-x); plot(cx+x,cy-y);
      ++y;
      if(err<0) err+=2*y+1;
      else { --x; err+=2*(y-x)+1; }
    }
  }

  void fillCircle(int x0,int y0,int r,uint16_t c){
    for(int y=-r;y<=r;y++){int xx=(int)sqrtf((float)r*r-(float)y*y);drawFastHLine(x0-xx,y0+y,2*xx+1,c);}
  }
  void drawCircle(int x0,int y0,int r,uint16_t c){
    int x=r,y=0,err=1-r;while(x>=y){drawPixel(x0+x,y0+y,c);drawPixel(x0+y,y0+x,c);drawPixel(x0-y,y0+x,c);drawPixel(x0-x,y0+y,c);drawPixel(x0-x,y0-y,c);drawPixel(x0-y,y0-x,c);drawPixel(x0+y,y0-x,c);drawPixel(x0+x,y0-y,c);y++;if(err<0)err+=2*y+1;else{x--;err+=2*(y-x)+1;}}
  }
  void setTextColor(uint16_t c){textColor_=c;}
  void setTextDatum(uint8_t d){datum_=d;}
  void drawString(const char* s,int x,int y,uint8_t font){
    int scale=font>=2?2:1;int len=strlen(s);int width=len*6*scale;
    if(datum_==1)x-=width/2; else if(datum_==2)x-=width;
    for(int n=0;s[n];n++){uint8_t col[5];glyph((char)s[n],col);for(int cx=0;cx<5;cx++)for(int cy=0;cy<7;cy++)if(col[cx]&(1<<cy))fillRect(x+n*6*scale+cx*scale,y+cy*scale,scale,scale,textColor_);}
  }
  const uint16_t* getPointer() const{return pixels_;}
  void push(){
    // Convert and transmit in small chunks: avoid a second 40 KB framebuffer.
    uint8_t chunk[256];
    SPI.beginTransaction(SPISettings(10000000,MSBFIRST,SPI_MODE0));digitalWrite(PIN_TFT_CS,LOW);
    commandNoCS(0x2A);data16(0);data16(W-1);commandNoCS(0x2B);data16(0);data16(H-1);commandNoCS(0x2C);
    for(int base=0;base<W*H;base+=128){
      int count=(W*H-base>128)?128:(W*H-base);
      for(int j=0;j<count;j++){uint16_t p=pixels_[base+j];chunk[j*2]=(uint8_t)(p>>8);chunk[j*2+1]=(uint8_t)p;}
      SPI.transferBytes(chunk,nullptr,(uint32_t)count*2);
    }
    digitalWrite(PIN_TFT_CS,HIGH);SPI.endTransaction();
  }
private:

  void drawPixelAlpha(int x,int y,uint16_t src,uint8_t a){
    if(x<0||x>=W||y<0||y>=H||a==0)return;
    if(a==255){pixels_[y*W+x]=src;return;}
    const uint16_t dst=pixels_[y*W+x]; const uint16_t ia=255-a;
    const uint16_t r=((((src>>11)&31)*a+((dst>>11)&31)*ia+127)/255);
    const uint16_t g=((((src>>5)&63)*a+((dst>>5)&63)*ia+127)/255);
    const uint16_t b=(((src&31)*a+(dst&31)*ia+127)/255);
    pixels_[y*W+x]=(r<<11)|(g<<5)|b;
  }
  uint16_t pixels_[W*H]={}; uint16_t textColor_=0xFFFF;uint8_t datum_=0;
  void commandNoCS(uint8_t c){digitalWrite(PIN_TFT_DC,LOW);SPI.transfer(c);digitalWrite(PIN_TFT_DC,HIGH);}
  void command(uint8_t c){digitalWrite(PIN_TFT_CS,LOW);digitalWrite(PIN_TFT_DC,LOW);SPI.transfer(c);digitalWrite(PIN_TFT_CS,HIGH);}
  void commandData(uint8_t c,uint8_t d){digitalWrite(PIN_TFT_CS,LOW);digitalWrite(PIN_TFT_DC,LOW);SPI.transfer(c);digitalWrite(PIN_TFT_DC,HIGH);SPI.transfer(d);digitalWrite(PIN_TFT_CS,HIGH);}
  void data16(uint16_t v){SPI.transfer(v>>8);SPI.transfer(v&255);}
  void drawFastHLineDummy(){}
  static void glyph(char ch,uint8_t* b){
    memset(b,0,5); if(ch>='a'&&ch<='z')ch-=32;
    switch(ch){
    case 'A':{uint8_t a[]={0x7E,0x09,0x09,0x09,0x7E};memcpy(b,a,5);break;}
    case 'B':{uint8_t a[]={0x7F,0x49,0x49,0x49,0x36};memcpy(b,a,5);break;}
    case 'C':{uint8_t a[]={0x3E,0x41,0x41,0x41,0x22};memcpy(b,a,5);break;}
    case 'D':{uint8_t a[]={0x7F,0x41,0x41,0x22,0x1C};memcpy(b,a,5);break;}
    case 'E':{uint8_t a[]={0x7F,0x49,0x49,0x49,0x41};memcpy(b,a,5);break;}
    case 'F':{uint8_t a[]={0x7F,0x09,0x09,0x09,0x01};memcpy(b,a,5);break;}
    case 'G':{uint8_t a[]={0x3E,0x41,0x49,0x49,0x7A};memcpy(b,a,5);break;}
    case 'H':{uint8_t a[]={0x7F,0x08,0x08,0x08,0x7F};memcpy(b,a,5);break;}
    case 'I':{uint8_t a[]={0x41,0x41,0x7F,0x41,0x41};memcpy(b,a,5);break;}
    case 'J':{uint8_t a[]={0x20,0x40,0x41,0x3F,0x01};memcpy(b,a,5);break;}
    case 'K':{uint8_t a[]={0x7F,0x08,0x14,0x22,0x41};memcpy(b,a,5);break;}
    case 'L':{uint8_t a[]={0x7F,0x40,0x40,0x40,0x40};memcpy(b,a,5);break;}
    case 'M':{uint8_t a[]={0x7F,0x02,0x0C,0x02,0x7F};memcpy(b,a,5);break;}
    case 'N':{uint8_t a[]={0x7F,0x04,0x08,0x10,0x7F};memcpy(b,a,5);break;}
    case 'O':{uint8_t a[]={0x3E,0x41,0x41,0x41,0x3E};memcpy(b,a,5);break;}
    case 'P':{uint8_t a[]={0x7F,0x09,0x09,0x09,0x06};memcpy(b,a,5);break;}
    case 'Q':{uint8_t a[]={0x3E,0x41,0x51,0x21,0x5E};memcpy(b,a,5);break;}
    case 'R':{uint8_t a[]={0x7F,0x09,0x19,0x29,0x46};memcpy(b,a,5);break;}
    case 'S':{uint8_t a[]={0x46,0x49,0x49,0x49,0x31};memcpy(b,a,5);break;}
    case 'T':{uint8_t a[]={0x01,0x01,0x7F,0x01,0x01};memcpy(b,a,5);break;}
    case 'U':{uint8_t a[]={0x3F,0x40,0x40,0x40,0x3F};memcpy(b,a,5);break;}
    case 'V':{uint8_t a[]={0x1F,0x20,0x40,0x20,0x1F};memcpy(b,a,5);break;}
    case 'W':{uint8_t a[]={0x3F,0x40,0x38,0x40,0x3F};memcpy(b,a,5);break;}
    case 'X':{uint8_t a[]={0x63,0x14,0x08,0x14,0x63};memcpy(b,a,5);break;}
    case 'Y':{uint8_t a[]={0x07,0x08,0x70,0x08,0x07};memcpy(b,a,5);break;}
    case 'Z':{uint8_t a[]={0x61,0x51,0x49,0x45,0x43};memcpy(b,a,5);break;}
    case '0':{uint8_t a[]={0x3E,0x51,0x49,0x45,0x3E};memcpy(b,a,5);break;}
    case '1':{uint8_t a[]={0x00,0x42,0x7F,0x40,0x00};memcpy(b,a,5);break;}
    case '2':{uint8_t a[]={0x62,0x51,0x49,0x49,0x46};memcpy(b,a,5);break;}
    case '3':{uint8_t a[]={0x22,0x49,0x49,0x49,0x36};memcpy(b,a,5);break;}
    case '4':{uint8_t a[]={0x18,0x14,0x12,0x7F,0x10};memcpy(b,a,5);break;}
    case '5':{uint8_t a[]={0x2F,0x49,0x49,0x49,0x31};memcpy(b,a,5);break;}
    case '6':{uint8_t a[]={0x3E,0x49,0x49,0x49,0x32};memcpy(b,a,5);break;}
    case '7':{uint8_t a[]={0x01,0x71,0x09,0x05,0x03};memcpy(b,a,5);break;}
    case '8':{uint8_t a[]={0x36,0x49,0x49,0x49,0x36};memcpy(b,a,5);break;}
    case '9':{uint8_t a[]={0x26,0x49,0x49,0x49,0x3E};memcpy(b,a,5);break;}
    case '-':b[1]=0x08;b[2]=0x08;b[3]=0x08;break;
    case '+':b[2]=0x1C;b[1]=0x08;b[3]=0x08;break;
    case '.':b[2]=0x60;break; case ':':b[2]=0x36;break; case '/':b[0]=0x60;b[1]=0x18;b[2]=0x06;break;
    case '!':b[2]=0x5F;break; case '?':{uint8_t a[]={0x02,0x01,0x51,0x09,0x06};memcpy(b,a,5);break;}
    case '%':{uint8_t a[]={0x63,0x13,0x08,0x64,0x63};memcpy(b,a,5);break;}
    case ' ':break; case '_':b[0]=b[1]=b[2]=b[3]=b[4]=0x40;break;
    default: b[0]=0x7F;b[4]=0x7F;break;
    }
  }
};
