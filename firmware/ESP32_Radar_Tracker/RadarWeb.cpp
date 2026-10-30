#include "RadarWeb.h"
#include <math.h>
static const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="ru"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>LD-RADAR</title><style>
body{margin:0;background:#080d12;color:#e8f2f7;font:16px system-ui}header{padding:14px;background:#101d29;border-bottom:2px solid #00d9ff}h1{margin:0;color:#00d9ff}main{max-width:900px;margin:auto;padding:12px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:8px}.card{background:#111c27;border:1px solid #294152;border-radius:9px;padding:12px;margin:10px 0}.v{font-size:23px;color:#7cff9b;font-weight:bold}label{display:block;margin-top:8px;color:#9db3c2}select,input,button{box-sizing:border-box;width:100%;padding:10px;margin:4px 0;border-radius:6px;border:1px solid #385467;background:#0b141d;color:white}button{background:#007f9e;border:0;font-weight:bold}.row{display:grid;grid-template-columns:1fr 1fr;gap:8px}canvas{width:100%;height:180px;background:#05090d}table{width:100%;border-collapse:collapse;font-size:13px}td,th{padding:6px;border-bottom:1px solid #243947;text-align:left}a{color:#00d9ff}.muted{color:#9db3c2}
</style><header><h1>LD-RADAR</h1><div class="muted">TTGO · журнал последних 80 измерений</div></header><main>
<div class="grid"><div class="card">Состояние<div id="state" class="v">—</div></div><div class="card">Дистанция<div id="dist" class="v">—</div></div><div class="card">Энергия<div id="energy" class="v">—</div></div><div class="card">OUT / кадры / ошибки<div id="counts" class="v">—</div></div></div>
<section class="card"><h3>Настройки дисплея</h3>
<label>Масштаб</label><select id="range"><option value="0">0,5 м</option><option value="1">1 м</option><option value="2">1,5 м</option><option value="3">2 м</option><option value="4">3 м</option><option value="5">4 м</option><option value="6">6 м</option></select>
<label>Яркость</label><select id="bright"><option value="0">1 — минимальная</option><option value="1">2</option><option value="2">3</option><option value="3">4</option><option value="4">5 — максимальная</option></select>
<label>Тема</label><select id="theme"><option value="0">GREEN</option><option value="1">AMBER</option><option value="2">CYAN</option></select>
<label>Стиль</label><select id="alien"><option value="1">ALIEN</option><option value="0">CLASSIC</option></select>
<label>Зона тревоги</label><select id="alert"><option value="0">OFF</option><option value="1">1 м</option><option value="2">2 м</option><option value="3">3 м</option></select>
<label>Шлейфы</label><select id="trails"><option value="1">ON</option><option value="0">OFF</option></select>
<label>Демонстрационный режим</label><select id="demo"><option value="0">OFF — реальные данные</option><option value="1">ON — DEMO</option></select>
<button onclick="saveSettings()">Применить настройки</button><p id="notice" class="muted">Настройки сохраняются в памяти устройства.</p></section>
<section class="card"><div class="row"><button onclick="togglePause()" id="pause">Приостановить обновление</button><button onclick="clearLog()">Очистить журнал</button></div><p><a href="/log.csv">Скачать CSV</a> · AP: 192.168.4.1</p></section>
<section class="card"><h3>График расстояния</h3><canvas id="chart" width="900" height="220"></canvas></section>
<section class="card"><h3>Домашний Wi-Fi</h3><form method="post" action="/wifi"><label>SSID</label><input name="ssid" maxlength="63"><label>Пароль</label><input name="password" type="password" maxlength="63"><button>Сохранить и подключить</button></form><p class="muted">Текущий STA: <span id="sta">—</span>. Точка доступа LD-RADAR остаётся включённой.</p></section>
<section class="card"><h3>Журнал измерений</h3><div style="overflow:auto;max-height:300px"><table><thead><tr><th>Время, с</th><th>Расстояние, м</th><th>Состояние</th><th>OUT</th><th>Энергия</th></tr></thead><tbody id="log"></tbody></table></div></section>
</main><script>
const $=id=>document.getElementById(id);let paused=false;
async function saveSettings(){let p=new URLSearchParams();['range','bright','theme','alien','alert','trails','demo'].forEach(k=>p.set(k,$(k).value));try{let r=await fetch('/settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:p});$('notice').textContent=r.ok?'Настройки применены и сохранены.':'Ошибка сохранения.';}catch(e){$('notice').textContent='Нет связи с устройством.'}}
async function loadSettings(){try{let r=await fetch('/settings');let d=await r.json();Object.keys(d).forEach(k=>{if($(k))$(k).value=String(d[k])})}catch(e){}}
function togglePause(){paused=!paused;$('pause').textContent=paused?'Продолжить обновление':'Приостановить обновление'}
async function clearLog(){if(!confirm('Очистить журнал и график?'))return;await fetch('/clear',{method:'POST'});refresh()}
function draw(a){let c=$('chart'),x=c.getContext('2d'),w=c.width,h=c.height;x.clearRect(0,0,w,h);x.strokeStyle='#294152';for(let i=0;i<=6;i++){let y=h-15-i*(h-30)/6;x.beginPath();x.moveTo(35,y);x.lineTo(w,y);x.stroke();x.fillStyle='#9db3c2';x.fillText(i+'м',2,y+4)}x.strokeStyle='#00d9ff';x.lineWidth=3;x.beginPath();let on=false;a.forEach((v,i)=>{if(v.cm<0||v.cm>600){on=false;return}let px=35+i*(w-40)/Math.max(1,a.length-1),py=h-15-v.cm/100/6*(h-30);if(!on){x.moveTo(px,py);on=true}else x.lineTo(px,py)});x.stroke()}
async function refresh(){if(paused)return;try{let r=await fetch('/api'),d=await r.json();$('state').textContent=d.state;$('dist').textContent=d.cm<0?'—':(d.cm/100).toFixed(2)+' м';$('energy').textContent=d.energy+'%';$('counts').textContent=d.out+' / '+d.frames+' / '+d.bad;$('sta').textContent=d.sta||'не подключено';draw(d.history);$('log').innerHTML=d.log.slice().reverse().map(v=>'<tr><td>'+v.sec+'</td><td>'+(v.cm<0?'—':(v.cm/100).toFixed(2))+'</td><td>'+v.state+'</td><td>'+v.out+'</td><td>'+v.energy+'</td></tr>').join('')}catch(e){$('state').textContent='Нет связи'}}
loadSettings();refresh();setInterval(refresh,1000);
</script></html>)HTML";
void RadarWeb::loadWifi(){wifiPrefs_.begin("radarwifi",true);ssid_=wifiPrefs_.getString("ssid","");password_=wifiPrefs_.getString("pass","");wifiPrefs_.end();}
void RadarWeb::connectSta(){if(ssid_.length()){WiFi.begin(ssid_.c_str(),password_.c_str());Serial.printf("[WiFi] Подключение к %s; AP остаётся активной\n",ssid_.c_str());}}
void RadarWeb::begin(UiSettings& s,bool& demo,RadarTargets& dt,RadarDisplay& display,Preferences& prefs){
 settings_=&s;demoMode_=&demo;demoTargets_=&dt;display_=&display;prefs_=&prefs;
 WiFi.mode(WIFI_AP_STA);WiFi.softAP("LD-RADAR");loadWifi();connectSta();
 server_.on("/",HTTP_GET,[this](){handleRoot();});server_.on("/api",HTTP_GET,[this](){handleApi();});
 server_.on("/settings",HTTP_GET,[this](){String j="{\"range\":"+String(settings_->rangeIdx)+",\"bright\":"+String(settings_->brightIdx)+",\"theme\":"+String(settings_->theme)+",\"alien\":"+String(settings_->alien?1:0)+",\"alert\":"+String(settings_->alertIdx)+",\"trails\":"+String(settings_->trails?1:0)+",\"demo\":"+String(*demoMode_?1:0)+"}";server_.send(200,"application/json",j);});
 server_.on("/settings",HTTP_POST,[this](){handleSettings();});
 server_.on("/log.csv",HTTP_GET,[this](){handleCsv();});
 server_.on("/clear",HTTP_POST,[this](){head_=count_=0;lastSample_=0;server_.send(200,"text/plain; charset=utf-8","Журнал очищен");});
 server_.on("/wifi",HTTP_POST,[this](){handleWifi();});
 server_.onNotFound([this](){server_.send(404,"text/plain","Страница не найдена");});server_.begin();
 Serial.printf("[WiFi] AP LD-RADAR at http://%s/\n",WiFi.softAPIP().toString().c_str());
}
void RadarWeb::loop(){server_.handleClient();}
int RadarWeb::currentDistance(const LD2401Data& d)const{uint16_t cm=0;if(d.state==1)cm=d.movDistCm;else if(d.state==2)cm=d.statDistCm;else if(d.state==3)cm=d.movDistCm?d.movDistCm:d.statDistCm;return cm>0&&cm<=600?(int)cm:-1;}
void RadarWeb::sample(uint32_t now,const RadarLD2401& radar,int out){frames_=radar.stats().framesOk;bad_=radar.stats().framesBad;staStatus_=WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():String("не подключено");if(now-lastSample_<1000&&count_)return;lastSample_=now;const LD2401Data& d=radar.data();uint8_t energy=d.state==2?d.statEnergy:d.movEnergy;rows_[head_]={now/1000,(int16_t)(radar.link(now)==LINK_OK?currentDistance(d):-1),d.state,(uint8_t)(out?1:0),energy};head_=(head_+1)%80;if(count_<80)count_++;}
void RadarWeb::handleRoot(){server_.send_P(200,"text/html; charset=utf-8",PAGE);}
void RadarWeb::handleSettings(){
 auto arg=[this](const char* k,int def){return server_.hasArg(k)?server_.arg(k).toInt():def;};
 settings_->rangeIdx=constrain(arg("range",settings_->rangeIdx),0,6);
 settings_->brightIdx=constrain(arg("bright",settings_->brightIdx),0,4);
 settings_->theme=constrain(arg("theme",settings_->theme),0,2);
 settings_->alien=arg("alien",settings_->alien?1:0)!=0;
 settings_->alertIdx=constrain(arg("alert",settings_->alertIdx),0,3);
 settings_->trails=arg("trails",settings_->trails?1:0)!=0;
 bool demo=arg("demo",*demoMode_?1:0)!=0;if(demo!=*demoMode_){*demoMode_=demo;demoTargets_->clear();Serial.println(demo?F("[UI] DEMO mode ON (web)"):F("[UI] DEMO mode OFF (web)"));}
 prefs_->putUChar("range",settings_->rangeIdx);prefs_->putUChar("bright",settings_->brightIdx);prefs_->putUChar("theme",settings_->theme);prefs_->putBool("trails",settings_->trails);prefs_->putBool("alien",settings_->alien);prefs_->putUChar("alert",settings_->alertIdx);
 display_->setBrightness(settings_->brightIdx);server_.send(200,"text/plain; charset=utf-8","OK");
}
void RadarWeb::handleApi(){
 String j;j.reserve(9000);const Row* latest=count_?&rows_[(head_+79)%80]:nullptr;
 j="{\"state\":\"";j+=latest?(latest->cm<0?"НЕТ ДАННЫХ":latest->state==0?"НЕТ ЦЕЛИ":latest->state==1?"ДВИЖЕНИЕ":latest->state==2?"НЕПОДВИЖНО":"ДВИЖ.+СТАТ."):"НЕТ ДАННЫХ";
 j+="\",\"cm\":";j+=latest?String(latest->cm):String(-1);j+=",\"energy\":";j+=latest?String(latest->energy):"0";j+=",\"out\":";j+=latest?String(latest->out):"0";j+=",\"frames\":"+String(frames_)+",\"bad\":"+String(bad_)+",\"sta\":\"";
 String sta=staStatus_;sta.replace("\\","\\\\");sta.replace("\"","\\\"");j+=sta;j+="\",\"history\":[";int start=(head_-count_+80)%80,n=count_<60?count_:60,skip=count_-n;
 for(int i=0;i<n;i++){if(i)j+=",";const Row&r=rows_[(start+skip+i)%80];j+="{\"cm\":"+String(r.cm)+",\"sec\":"+String(r.ms)+"}";}
 j+="],\"log\":[";for(int i=0;i<count_;i++){if(i)j+=",";const Row&r=rows_[(start+i)%80];const char* st=r.cm<0?"НЕТ ДАННЫХ":r.state==0?"НЕТ ЦЕЛИ":r.state==1?"ДВИЖЕНИЕ":r.state==2?"НЕПОДВИЖНО":"ДВИЖ.+СТАТ.";j+="{\"sec\":"+String(r.ms)+",\"cm\":"+String(r.cm)+",\"state\":\""+st+"\",\"out\":"+String(r.out)+",\"energy\":"+String(r.energy)+"}";}j+="]}";server_.send(200,"application/json",j);
}
void RadarWeb::handleCsv(){String s="время_с,расстояние_см,состояние,out,энергия_процентов\r\n";int start=(head_-count_+80)%80;for(int i=0;i<count_;i++){const Row&r=rows_[(start+i)%80];s+=String(r.ms)+","+String(r.cm)+","+String(r.state)+","+String(r.out)+","+String(r.energy)+"\r\n";}server_.sendHeader("Content-Disposition","attachment; filename=esp32-radar-zhurnal.csv");server_.send(200,"text/csv; charset=utf-8",s);}
void RadarWeb::handleWifi(){ssid_=server_.arg("ssid");password_=server_.arg("password");ssid_.trim();if(ssid_.length()){wifiPrefs_.begin("radarwifi",false);wifiPrefs_.putString("ssid",ssid_);wifiPrefs_.putString("pass",password_);wifiPrefs_.end();WiFi.mode(WIFI_AP_STA);connectSta();}server_.sendHeader("Location","/");server_.send(303,"text/plain","Сохранено. Точка доступа остаётся активной.");}
