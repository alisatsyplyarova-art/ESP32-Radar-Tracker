#include "RadarTargets.h"
#include <math.h>
void RadarTargets::clear() { for (auto& t : t_) t = TrackedTarget(); }
bool RadarTargets::alive(const TrackedTarget& t, uint32_t now) { return t.active && (now - t.lastSeenMs) <= TARGET_TIMEOUT_MS; }

// Match all three LD2450 detections to all living tracks together. Predicting
// positions and minimizing the global assignment cost prevents greedy ID swaps.
void RadarTargets::update(const LD2450Target in[3], uint32_t now) {
  int8_t trackOf[3] = {-1, -1, -1};
  bool used[3] = {false, false, false};
  float px[3] = {0,0,0}, py[3] = {0,0,0};
  int32_t gate2[3] = {0,0,0};
  for (int j=0;j<3;j++) if (alive(t_[j],now)) {
    uint32_t age=now-t_[j].lastSeenMs;
    float dt=(age>700?700.0f:(float)age)/1000.0f;
    px[j]=t_[j].rawX+t_[j].vx*dt; py[j]=t_[j].rawY+t_[j].vy*dt;
    float gate=950.0f+fabsf((float)t_[j].speed)*10.0f*dt;
    if(gate>1900.0f) gate=1900.0f;
    gate2[j]=(int32_t)(gate*gate);
  }
  int bestN=-1; int32_t bestCost=INT32_MAX; int8_t best[3]={-1,-1,-1};
  // -1 means a detection has no match. At most 4^3 assignments are checked.
  for(int a=-1;a<3;a++) for(int b=-1;b<3;b++) for(int c=-1;c<3;c++) {
    int8_t cand[3]={(int8_t)a,(int8_t)b,(int8_t)c};
    bool ok=true; int n=0; int32_t cost=0;
    for(int i=0;i<3 && ok;i++) {
      if(!in[i].valid) { if(cand[i]!=-1) ok=false; continue; }
      int j=cand[i]; if(j<0) continue;
      if(!alive(t_[j],now)) {ok=false;break;}
      for(int k=0;k<i;k++) if(cand[k]==j) ok=false;
      if(!ok) break;
      float dx=(float)in[i].x-px[j], dy=(float)in[i].y-py[j];
      int32_t d2=(int32_t)(dx*dx+dy*dy);
      if(d2>gate2[j]) {ok=false;break;}
      cost+=d2; n++;
    }
    if(ok && (n>bestN || (n==bestN && cost<bestCost))) {
      bestN=n; bestCost=cost;
      for(int i=0;i<3;i++) best[i]=cand[i];
    }
  }
  for(int i=0;i<3;i++) if(in[i].valid && best[i]>=0) {
    trackOf[i]=best[i]; used[(uint8_t)best[i]]=true;
  }
  // Unmatched detections use expired slots first, then recycle the oldest
  // unmatched track and clear its old smoothing/trail state.
  for(int i=0;i<3;i++) if(in[i].valid && trackOf[i]<0) {
    int pick=-1;
    for(int j=0;j<3 && pick<0;j++) if(!used[j] && !alive(t_[j],now)) pick=j;
    if(pick<0) {
      uint32_t oldest=0;
      for(int j=0;j<3;j++) if(!used[j]) {
        uint32_t age=now-t_[j].lastSeenMs;
        if(pick<0 || age>oldest) {pick=j;oldest=age;}
      }
    }
    if(pick<0) continue;
    trackOf[i]=(int8_t)pick; used[pick]=true; t_[pick].active=false;
  }
  for(int i=0;i<3;i++) {
    if(!in[i].valid || trackOf[i]<0) continue;
    TrackedTarget& t=t_[trackOf[i]];
    bool fresh=!t.active || (now-t.lastSeenMs)>TARGET_TIMEOUT_MS;
    if(fresh) {
      t.sx=in[i].x; t.sy=in[i].y; t.vx=0; t.vy=0;
      t.trailCount=0; t.trailHead=0; t.lastTrailMs=now-333UL;
    } else {
      uint32_t elapsed=now-t.lastSeenMs;
      if(elapsed>=20) {
        float dt=(float)elapsed/1000.0f;
        float vx=((float)in[i].x-t.rawX)/dt, vy=((float)in[i].y-t.rawY)/dt;
        if(vx>6000)vx=6000; if(vx< -6000)vx=-6000;
        if(vy>6000)vy=6000; if(vy< -6000)vy=-6000;
        t.vx=t.vx*0.55f+vx*0.45f; t.vy=t.vy*0.55f+vy*0.45f;
      }
      t.sx+=TARGET_SMOOTH*((float)in[i].x-t.sx);
      t.sy+=TARGET_SMOOTH*((float)in[i].y-t.sy);
    }
    t.rawX=in[i].x; t.rawY=in[i].y; t.speed=in[i].speed;
    t.res=in[i].res; t.rawDist=in[i].dist;
    // Keep a 5-minute history without storing every 10 Hz UART update.
    if((uint32_t)(now-t.lastTrailMs)>=333UL){
      t.trail[t.trailHead]={(int16_t)t.sx,(int16_t)t.sy,(uint16_t)(now/100UL)};
      t.trailHead=(t.trailHead+1)%TRAIL_LEN;
      if(t.trailCount<TRAIL_LEN)t.trailCount++;
      t.lastTrailMs=now;
    }
    t.active=true; t.lastSeenMs=now;
  }
}
void RadarTargets::expire(uint32_t now) {
  for(auto& t:t_) if(t.active && (now-t.lastSeenMs)>TARGET_TIMEOUT_MS) t.active=false;
}
TargetState RadarTargets::stateOf(uint8_t i,uint32_t now) const {
  const TrackedTarget& t=t_[i]; if(!t.active)return TS_NONE;
  uint32_t age=now-t.lastSeenMs; if(age>TARGET_TIMEOUT_MS)return TS_NONE;
  return age<=TARGET_HOLD_MS?TS_LIVE:TS_HOLD;
}
uint8_t RadarTargets::liveCount(uint32_t now) const {
  uint8_t n=0; for(uint8_t i=0;i<3;i++)if(stateOf(i,now)==TS_LIVE)n++; return n;
}
int8_t RadarTargets::nearest(uint32_t now) const {
  int8_t best=-1; float bd=1e9f;
  for(uint8_t i=0;i<3;i++)if(stateOf(i,now)==TS_LIVE && t_[i].rawDist<bd){bd=t_[i].rawDist;best=i;}
  return best;
}
bool RadarTargets::trailAt(const TrackedTarget& t,uint16_t k,TrailPoint& out) {
  if(k>=t.trailCount)return false;
  int idx=((int32_t)t.trailHead-1-(int32_t)k+2*TRAIL_LEN)%TRAIL_LEN; out=t.trail[idx]; return true;
}
