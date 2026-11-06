#include "RadarWeb.h"
#include "Config.h"
#include "Version.h"
#include <math.h>

static const char PAGE[] PROGMEM = R"HTML(<!doctype html><html lang="ru"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>LD-RADAR</title><style>
:root{color-scheme:dark}body{margin:0;background:#080d12;color:#e8f2f7;font:16px system-ui,sans-serif}header{background:#101d29;padding:16px;border-bottom:2px solid #00d9ff}h1{margin:0;color:#00d9ff;font-size:24px}main{max-width:980px;margin:auto;padding:14px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:10px}.card{background:#111c27;border:1px solid #294152;border-radius:10px;padding:12px}.v{font-size:24px;color:#7cff9b;font-weight:700}.topnav{display:flex;gap:8px;flex-wrap:wrap;margin:12px 0}.topnav button{width:auto;flex:1;min-width:130px}.page{display:none}.page.active{display:block}.wifi-top{display:grid;grid-template-columns:minmax(150px,1fr) minmax(220px,2fr) minmax(100px,1fr);align-items:center;gap:10px;margin:12px 0}.wifi-form{display:grid;grid-template-columns:1fr;gap:6px}.wifi-form form{display:grid;grid-template-columns:1fr 1fr auto;gap:6px}.wifi-form input,.wifi-form button,.wifi-form select{margin:0}.wifi-scan{display:grid;grid-template-columns:1fr auto;gap:6px}.wifi-scan button{width:auto;white-space:nowrap}@media(max-width:650px){.wifi-top{grid-template-columns:1fr}.wifi-form form{grid-template-columns:1fr}.wifi-scan{grid-template-columns:1fr}.topnav button{min-width:100%}}canvas{width:100%;height:220px;background:#05090d;border-radius:8px}.radar-controls{display:flex;align-items:center;justify-content:center;gap:9px;flex-wrap:wrap;margin-top:9px}.radar-control{display:flex;align-items:center;gap:6px;background:#0b141d;border:1px solid #385467;border-radius:7px;padding:6px 9px;margin:0;font-size:13px;white-space:nowrap}.radar-control input{appearance:none;-webkit-appearance:none;position:relative;width:30px;min-width:30px;height:16px;margin:0;padding:0;border:0;border-radius:9px;background:#40515f;cursor:pointer;transition:background .15s}.radar-control input::after{content:"";position:absolute;left:2px;top:2px;width:12px;height:12px;border-radius:50%;background:#fff;transition:transform .15s}.radar-control input:checked{background:#00a9c7}.radar-control input:checked::after{transform:translateX(14px)}.radar-control select{width:auto;min-width:72px;padding:5px 7px;margin:0;font-size:13px}.legend{display:flex;justify-content:center;gap:14px;flex-wrap:wrap;margin-top:8px;font-size:13px;color:#c5d5df}.legend span{display:inline-flex;align-items:center;gap:5px}.legend i{display:inline-block;width:15px;height:4px;border-radius:3px}.screen-copy{display:block;width:min(100%,720px);height:auto;aspect-ratio:16/9;margin:10px auto;background:#080d12;border:1px solid #294152;border-radius:8px;image-rendering:pixelated}.screen-caption{text-align:center;color:#9db3c2;font-size:13px}.muted{color:#9db3c2}input,button,select{box-sizing:border-box;width:100%;padding:11px;margin:5px 0;border-radius:6px;border:1px solid #385467;background:#0b141d;color:#fff}button{background:#007f9e;border:0;font-weight:700}label{display:block;margin-top:8px;color:#b9ceda}input[type=checkbox]{width:auto;min-width:20px;vertical-align:middle}select{cursor:pointer}a{color:#00d9ff}.row{display:flex;gap:8px}.row>*{flex:1}table{border-collapse:collapse;width:100%;font-size:13px}td,th{padding:6px;border-bottom:1px solid #243947;text-align:left}section{margin:16px 0}.ok{color:#7cff9b}.bad{color:#ff7b72}</style></head><body><header><h1>LD-RADAR</h1><div class="muted">Firmware: <b id="firmwareVersion">—</b> · TTGO · датчик: <span id="sensor">—</span> · 60 секунд · <a href="#" onclick="showJournal();return false">Журнал</a></div></header><main><nav class="topnav"><button type="button" onclick="showPage('home')">Радар</button><button type="button" onclick="showPage('settings')">Устройство / зоны / журнал</button><button type="button" onclick="showPage('help')">Инструкция</button></nav><div class="wifi-top card"><div><b>Домашний Wi-Fi</b><div class="muted">Точка доступа 192.168.4.1 остаётся активной</div></div><div class="wifi-form"><div class="wifi-scan"><select id="wifiNetworks" onchange="$('ssidInput').value=this.value"><option value="">Выберите найденную сеть…</option></select><button type="button" onclick="scanWifi()">Найти сети</button></div><form method="post" action="/wifi"><input id="ssidInput" name="ssid" maxlength="63" autocomplete="off" placeholder="SSID (можно ввести скрытую сеть)"><input name="password" type="password" maxlength="63" autocomplete="new-password" placeholder="Пароль Wi-Fi"><button type="submit">Сохранить и подключить</button></form></div><div class="muted">STA: <span id="sta">—</span></div></div><div id="homePage" class="page active">
<div class="grid"><div class="card">Состояние<div id="state" class="v">—</div></div><div class="card">Дистанция<div id="dist" class="v">—</div></div><div class="card">Энергия<div id="energy" class="v">—</div></div><div class="card">Зона оповещения<div id="alarm" class="v">—</div></div><div class="card">OUT / кадры / ошибки<div id="counts" class="v">—</div></div></div>
<section class="card"><h3>LD2450 — веб-радар (вид сверху)</h3><canvas id="webRadar" width="900" height="400" style="height:auto;aspect-ratio:9/4"></canvas><div class="radar-controls"><label class="radar-control"><input id="webTrailsSel" type="checkbox" checked onchange="act('webtrails',this.checked?1:0)"> Шлейф</label><label class="radar-control"><input id="rearFilterSel" type="checkbox" onchange="act('rearfilter',this.checked?1:0)"> Не фильтровать сзади</label><label class="radar-control">Масштаб <select id="webScaleSel" onchange="act('webscale',this.value)"><option value="0">2 м</option><option value="1">4 м</option><option value="2" selected>6 м</option></select></label></div><div class="legend"><span><i style="background:#ff9f43"></i>T1</span><span><i style="background:#b56bff"></i>T2</span><span><i style="background:#9cff57"></i>T3</span><span><i style="background:#377dff"></i>Шлейф</span></div></section>
<section class="card"><h3>Фильтр расстояния</h3><p class="muted">Цели ближе минимума или дальше максимума полностью исключаются из регистрации, карты и тревог.</p><div class="row"><label>Минимум, м<input id="minRange" type="number" min="0.1" max="20" step="0.1" value="0.5"></label><label>Максимум, м<input id="maxRange" type="number" min="0.2" max="20" step="0.1" value="5.0"></label></div><button type="button" onclick="saveDistanceFilter()">Применить диапазон</button><div class="muted" id="rangeStatus">Рабочий диапазон: 0,5–5 м</div></section>
<section class="card"><h3>Расстояние до каждой цели — последние 60 секунд</h3><canvas id="chart" width="900" height="260"></canvas><div id="chartLegend" class="muted">T1 — оранжевая · T2 — фиолетовая · T3 — зелёная. Для LD2401 отображается одна линия.</div></section>
<section class="card"><h3>Управление</h3><div class="row"><button type="button" id="pause" onclick="togglePause()">Приостановить обновление</button><button type="button" onclick="clearLog()">Очистить журнал</button></div><p id="notice" class="muted">Обновление данных включено. Очистка удаляет только журнал в оперативной памяти до перезапуска.</p></section>
</div><div id="settingsPage" class="page"><section class="card"><h3>Управление устройством</h3><div class="muted" id="uistate">Загрузка настроек…</div>
<label for="screenSel">Экран TFT</label><select id="screenSel" onchange="act('screen',this.value)"><option value="0">RADAR — обзор</option><option value="1">TARGETS — цели</option><option value="2">PRESENCE — присутствие</option><option value="3">DUAL — двойной экран</option><option value="4">DIAG — диагностика</option><option value="5">SETTINGS — настройки</option><option value="6">WIFI — сеть и IP</option></select>
<label for="rangeSel">Масштаб радара</label><select id="rangeSel" onchange="act('range',this.value)"><option value="3">2 м</option><option value="5">4 м</option><option value="6">6 м</option></select>
<label for="brightSel">Яркость экрана</label><select id="brightSel" onchange="act('bright',this.value)"><option value="0">1 — минимальная</option><option value="1">2</option><option value="2">3 — средняя</option><option value="3">4</option><option value="4">5 — максимальная</option></select>
<label for="themeSel">Цветовая схема</label><select id="themeSel" onchange="act('theme',this.value)"><option value="0">Зелёная</option><option value="1">Янтарная</option><option value="2">Голубая</option></select>

<label for="alertSel">Зона оповещения</label><select id="alertSel" onchange="act('alert',this.value)"><option value="0">Выключена</option><option value="1">1 м</option><option value="2">2 м</option><option value="3">3 м</option></select>
<label for="sweepSel">Луч сканирования</label><select id="sweepSel" onchange="act('sweep',this.value)"><option value="0">Выключен</option><option value="1">Включён</option></select>
<label for="trailsSel">Следы движения</label><select id="trailsSel" onchange="act('trails',this.value)"><option value="1">Включены</option><option value="0">Выключены</option></select>
<label for="demoSel">Демонстрационный режим</label><select id="demoSel" onchange="act('demo',this.value)"><option value="0">Выключен — реальные данные</option><option value="1">Включён — тестовые цели</option></select>
<label for="buzzerSel">Внешнее звуковое оповещение</label><select id="buzzerSel" onchange="act('buzzer',this.value)"><option value="0">Выключено</option><option value="1">Включено при тревоге</option></select>

<div class="card" style="margin-top:12px"><h3>Зоны обнаружения LD2450</h3><p class="muted">Карта сверху: датчик внизу, Y+ — перед ним. Переключатели влияют только на тревогу — цели продолжают отображаться.</p><canvas id="zoneMap" width="600" height="360" style="height:260px"></canvas><div class="grid"><label><input type="checkbox" id="z0" onchange="zonesChanged()"> Передняя центральная</label><label><input type="checkbox" id="z1" onchange="zonesChanged()"> Левая</label><label><input type="checkbox" id="z2" onchange="zonesChanged()"> Правая</label><label><input type="checkbox" id="z3" onchange="zonesChanged()"> Задняя</label></div><div class="muted">Передняя центральная зона включена по умолчанию.</div></div>
<p class="muted">Для звукового оповещения нужен активный зуммер с отдельным транзисторным ключом на GPIO32. Не подключайте нагрузку напрямую к выводу ESP32.</p>
<p class="muted">Выбирайте нужную функцию напрямую — физические кнопки B1/B2 через веб больше не эмулируются. Изменения применяются сразу; параметры дисплея сохраняются в памяти устройства.</p></section><section class="card" id="journalSection"><h3>Журнал измерений</h3><p><a href="/log.csv">Скачать последние 80 измерений (CSV)</a></p><div style="overflow:auto;max-height:330px"><table><thead><tr><th>Время, с</th><th>Расстояние, м</th><th>Состояние</th><th>OUT</th><th>Энергия</th><th>Тревога</th></tr></thead><tbody id="log"></tbody></table></div></section></div><div id="helpPage" class="page"><section class="card"><h3>Инструкция: радар и кнопки</h3>
<p><b>Состояние:</b> «НЕТ ДАННЫХ» — измерение не получено или связь с датчиком потеряна; «НЕТ ЦЕЛИ» — цель не обнаружена; «ДВИЖЕНИЕ» — обнаружена движущаяся цель; «НЕПОДВИЖНО» — обнаружена неподвижная цель; «ДВИЖ.+СТАТ.» — датчик сообщает о движущейся и неподвижной составляющих одновременно.</p>
<p><b>Дистанция</b> — расстояние до обнаруженной цели в метрах. <b>Энергия</b> — уровень сигнала/энергии обнаружения в процентах. <b>OUT / кадры / ошибки</b> — состояние выходного сигнала OUT, число принятых кадров и число ошибочных кадров.</p>
<p><b>Зона оповещения</b> показывает «ТРЕВОГА», когда ближайшая обнаруженная цель находится внутри выбранной зоны 1, 2 или 3 м. При выключенной зоне или отсутствии достоверной цели отображается «Нет».</p><p><b>График</b> показывает расстояние за последние 60 секунд; шкала по вертикали — метры, по горизонтали — секунды. Разрывы означают, что в этот момент достоверное расстояние не было доступно.</p>
<p><b>Приостановить обновление / Продолжить обновление</b> — останавливает или возобновляет обновление данных в браузере. Сам датчик и измерения на устройстве продолжают работать.</p>
<p><b>Очистить журнал</b> — очищает накопленные веб-журнал и график в оперативной памяти устройства. Это действие нельзя отменить; после перезапуска журнал в любом случае начинается заново. Для сохранения данных сначала скачайте CSV.</p>
<p><b>Скачать CSV</b> — сохраняет последние измерения в файл, который можно открыть в Excel. <b>Сохранить и подключить</b> — сохраняет SSID и пароль домашнего Wi‑Fi; точка доступа LD-RADAR остаётся доступной по адресу 192.168.4.1.</p>
<p><b>Управление устройством:</b> выпадающие списки напрямую выбирают экран, масштаб, яркость, цветовую схему, стиль целей, зону оповещения, следы и DEMO. Это не имитация нажатий физических кнопок. Настройки чувствительности самого датчика LD2401/LD2450 здесь не меняются.</p></section></div>

<!--WIFI_SLOT-->
<!--HELP_SLOT-->

</main><script>
const $=id=>document.getElementById(id);let paused=false;async function scanWifi(){const s=$('wifiNetworks');s.innerHTML='<option value="">Сканирование…</option>';try{const r=await fetch('/scan');if(!r.ok)throw 0;const d=await r.json();s.innerHTML='';if(!d.networks||!d.networks.length){s.innerHTML='<option value="">Сети не найдены</option>';return;}d.networks.forEach(n=>{const o=document.createElement('option');o.value=n.ssid;o.textContent=n.ssid+' ('+n.rssi+' dBm)'+(n.secure?' 🔒':'');s.appendChild(o);});}catch(e){s.innerHTML='<option value="">Ошибка сканирования</option>';}}function showPage(name){document.querySelectorAll('.page').forEach(p=>p.classList.remove('active'));const el=$(name+'Page');if(el)el.classList.add('active');window.scrollTo(0,0);}function showJournal(){showPage('settings');setTimeout(()=>{const el=$('journalSection');if(el)el.scrollIntoView({behavior:'smooth',block:'start'});},50);}
// Independent web radar renderer: does not copy the small TFT framebuffer.
const webRadar=$('webRadar'),webRadarCtx=webRadar.getContext('2d');
const targetColors=['#ff9f43','#b56bff','#9cff57'];let webTrailsEnabled=true,showRearTargets=false,webRadarRangeM=6,webSweepEnabled=false;
function drawWebRadar(ts){
 const g=webRadarCtx,w=webRadar.width,h=webRadar.height,cx=w/2,cy=320,scale=280/(webRadarRangeM*1000),maxR=webRadarRangeM*1000*scale;
 g.clearRect(0,0,w,h);g.fillStyle='#071019';g.fillRect(0,0,w,h);g.strokeStyle='#294152';g.lineWidth=2;
 for(let r=1000;r<=webRadarRangeM*1000;r+=1000){let rr=r*scale;g.beginPath();g.arc(cx,cy,rr,Math.PI,Math.PI*2);g.stroke();if(showRearTargets){g.setLineDash([5,7]);g.beginPath();g.arc(cx,cy,rr,0,Math.PI);g.stroke();g.setLineDash([]);}g.fillStyle='#9db3c2';g.font='16px system-ui';g.fillText((r/1000)+' м',cx+8,cy-rr+18)}
 g.beginPath();g.moveTo(cx-450,cy);g.lineTo(cx+450,cy);g.moveTo(cx,cy-maxR);g.lineTo(cx,cy-maxR);g.strokeStyle='#426174';g.stroke();
 g.fillStyle='#d6e5ef';g.beginPath();g.arc(cx,cy,9,0,Math.PI*2);g.fill();g.fillStyle='#9db3c2';g.fillText('ДАТЧИК',cx+16,cy-8);g.fillText('ПЕРЕД',cx+12,26);if(showRearTargets)g.fillText('ЗАД',cx+12,h-14);
 if(webSweepEnabled){
  const phase=(performance.now()%4200)/4200;
  const angle=phase*Math.PI;
  const beamLen=maxR;
  g.save();
  g.beginPath();
  g.moveTo(cx,cy);
  g.lineTo(cx+Math.cos(angle)*beamLen,cy-Math.sin(angle)*beamLen);
  g.strokeStyle='rgba(0,210,255,0.95)';
  g.lineWidth=3;
  g.shadowColor='rgba(0,210,255,0.45)';
  g.shadowBlur=5;
  g.stroke();
  g.restore();
 }
 for(let i=0;i<3;i++){const t=(ts||[])[i];if(!webTrailsEnabled||!t||!t.valid||!Array.isArray(t.trail)||t.trail.length<2)continue;
  const pts=t.trail.slice().reverse().filter(p=>showRearTargets||p[1]>=0).map(p=>({x:cx+p[0]*scale,y:cy-p[1]*scale}));
  g.lineWidth=4;g.lineCap='round';g.lineJoin='round';
  for(let k=1;k<pts.length;k++){const a=pts[k-1],b=pts[k];if(a.x<0||a.x>w||b.x<0||b.x>w||a.y<0||a.y>h||b.y<0||b.y>h)continue;g.beginPath();g.moveTo(a.x,a.y);g.lineTo(b.x,b.y);g.strokeStyle='rgba(55,125,255,'+(0.35+0.6*k/pts.length)+')';g.stroke();}
  for(let k=0;k<pts.length;k++){const q=pts[k];if(q.x>=0&&q.x<=w&&q.y>=0&&q.y<=h){g.beginPath();g.arc(q.x,q.y,3,0,Math.PI*2);g.fillStyle='rgba(55,125,255,'+(0.3+0.65*k/pts.length)+')';g.fill();}}
 }
 for(let i=0;i<3;i++){const t=(ts||[])[i];if(!t||!t.valid||(!showRearTargets&&t.y<0))continue;const x=cx+t.x*scale,y=cy-t.y*scale;if(x<8||x>w-8||y<8||y>h-8)continue;g.beginPath();g.arc(x,y,12,0,Math.PI*2);g.fillStyle=targetColors[i];g.fill();g.lineWidth=3;g.strokeStyle='#f2f7fb';g.stroke();g.fillStyle=targetColors[i];g.font='bold 20px system-ui';g.fillText('T'+(i+1),x+17,y-12);g.font='16px system-ui';g.fillText((Math.hypot(t.x,t.y)/1000).toFixed(2)+' м',x+17,y+12)}
}
function togglePause(){paused=!paused;$('pause').textContent=paused?'Продолжить обновление':'Приостановить обновление';$('notice').textContent=paused?'Обновление в браузере приостановлено; радар продолжает работать.':'Обновление данных включено.';}async function clearLog(){if(!confirm('Очистить журнал и график в памяти устройства? Сначала скачайте CSV, если данные нужны.'))return;try{let r=await fetch('/clear',{method:'POST'});$('notice').textContent=r.ok?'Журнал очищен.':'Не удалось очистить журнал.';refresh();}catch(e){$('notice').textContent='Ошибка связи при очистке журнала.';}}function draw(a,tracks,sensor){
 const c=$('chart'),g=c.getContext('2d'),w=c.width,h=c.height;g.clearRect(0,0,w,h);g.strokeStyle='#233c4b';g.fillStyle='#9db3c2';g.font='14px sans-serif';
 for(let i=0;i<=6;i++){let y=h-25-i*(h-45)/6;g.beginPath();g.moveTo(45,y);g.lineTo(w-8,y);g.stroke();g.fillText(i+' м',4,y+4)}
 for(let i=0;i<6;i++){let xx=45+i*(w-55)/5;g.fillText((i*10)+' с',xx,h-5)}
 const plot=(values,color)=>{g.strokeStyle=color;g.lineWidth=3;g.beginPath();let started=false;for(let i=0;i<values.length;i++){let v=values[i];if(v<0||v>600){started=false;continue;}let px=45+i*(w-55)/Math.max(1,values.length-1),py=h-25-(v/100)*(h-45)/6;if(!started){g.moveTo(px,py);started=true;}else g.lineTo(px,py);}g.stroke();};
 if(sensor==='LD2450'&&Array.isArray(tracks)){for(let i=0;i<3;i++)plot(tracks[i]||[],targetColors[i]);$('chartLegend').textContent='LD2450: T1 — оранжевая · T2 — фиолетовая · T3 — зелёная. Пропуски измерений разрывают соответствующую линию.';}
 else {plot((a||[]).map(v=>v.cm),'#00d9ff');$('chartLegend').textContent='LD2401: расстояние до зоны присутствия. Для LD2450 отображаются три отдельные линии целей.';}
}
const SCR=['RADAR','TARGETS','PRESENCE','DUAL','DIAG','SETTINGS','WIFI'],RNG=['0.5','1','1.5','2','3','4','6'],TH=['Зелёная','Янтарная','Голубая'],AL=['Выкл.','1 м','2 м','3 м'];let uiLoaded=false, lastTargets=[], currentMinCm=50, currentMaxCm=500;
function setVal(id,v){const e=$(id);if(e&&document.activeElement!==e&&String(e.value)!==String(v))e.value=String(v)}
function drawUi(u){webSweepEnabled=!!u.sweep;webTrailsEnabled=!!u.webTrails;showRearTargets=!!u.rearFilterBypass;webRadarRangeM=[2,4,6][u.webScaleIdx]||6;$('webTrailsSel').checked=!!u.webTrails;$('rearFilterSel').checked=!!u.rearFilterBypass;setVal('webScaleSel',u.webScaleIdx);currentMinCm=u.minCm;currentMaxCm=u.maxCm;setVal('minRange',(u.minCm/100).toFixed(1));setVal('maxRange',(u.maxCm/100).toFixed(1));setVal('screenSel',u.screen);setVal('rangeSel',u.range);setVal('brightSel',u.bright);setVal('themeSel',u.theme);setVal('alertSel',u.alert);setVal('trailsSel',u.trails?1:0);setVal('demoSel',u.demo?1:0);setVal('buzzerSel',u.buzzer?1:0);setVal('sweepSel',u.sweep?1:0);for(let i=0;i<4;i++)$('z'+i).checked=!!(u.zones&(1<<i));let vals='Экран: '+SCR[u.screen]+' · масштаб: '+RNG[u.range]+' м · яркость: '+(u.bright+1)+'/5 · схема: '+TH[u.theme]+'+' · оповещение: '+AL[u.alert]+' · следы: '+(u.trails?'вкл.':'выкл.')+' · DEMO: '+(u.demo?'вкл.':'выкл.')+' · зуммер: '+(u.buzzer?'вкл.':'выкл.')+' · маска зон: '+(u.zones||0); $('uistate').textContent=vals;uiLoaded=true}
async function saveDistanceFilter(){let mn=Math.round(Number($('minRange').value)*100),mx=Math.round(Number($('maxRange').value)*100);if(!Number.isFinite(mn)||!Number.isFinite(mx)||mn<10||mx>2000||mn>=mx){$('rangeStatus').textContent='Ошибка: укажите минимум от 0,1 м и максимум больше минимума, не более 20 м.';return}if(mx>currentMaxCm){await act('maxrange',mx);await new Promise(r=>setTimeout(r,250));await act('minrange',mn)}else{await act('minrange',mn);await new Promise(r=>setTimeout(r,250));await act('maxrange',mx)}$('rangeStatus').textContent='Рабочий диапазон: '+(mn/100).toFixed(1)+'–'+(mx/100).toFixed(1)+' м';}
function zonesChanged(){let m=0;for(let i=0;i<4;i++)if($('z'+i).checked)m|=1<<i;act('zones',m)}
function drawZoneMap(ts){let c=$('zoneMap'),g=c.getContext('2d'),w=c.width,h=c.height;g.clearRect(0,0,w,h);g.fillStyle='#071019';g.fillRect(0,0,w,h);let cx=w/2,cy=h/2,scale=0.045;g.strokeStyle='#345064';g.lineWidth=1;for(let r=1000;r<=4000;r+=1000){g.beginPath();g.arc(cx,cy,r*scale,0,Math.PI*2);g.stroke();g.fillStyle='#9db3c2';g.fillText((r/1000)+'m',cx+5,cy-r*scale)}g.beginPath();g.moveTo(cx-205,cy);g.lineTo(cx+205,cy);g.moveTo(cx,cy-165);g.lineTo(cx,cy+165);g.strokeStyle='#426174';g.stroke();g.fillStyle='#d6e5ef';g.fillRect(cx-7,cy-7,14,14);g.fillStyle='#00d9ff';g.fillText('ДАТЧИК',cx+12,cy-10);g.fillStyle='#9db3c2';g.fillText('ПЕРЕД',cx+10,18);g.fillText('ЗАД',cx+10,h-8);for(const t of (ts||[])){if(!t.valid)continue;let x=cx+t.x*scale,y=cy-t.y*scale;if(x<4||x>w-4||y<4||y>h-10)continue;g.beginPath();g.arc(x,y,7,0,Math.PI*2);g.fillStyle=targetColors[ts.indexOf(t)]||targetColors[0];g.fill();g.strokeStyle='#fff';g.stroke()}}
async function act(name,value){try{let r=await fetch('/action?n='+encodeURIComponent(name)+'&v='+encodeURIComponent(value),{method:'POST'});if(!r.ok)throw 0;$('notice').textContent='Применено: '+name+'.';setTimeout(()=>refresh(true),150)}catch(e){$('notice').textContent='Не удалось применить настройку.';refresh(true)}}
async function refresh(f){if(paused&&!f)return;try{let r=await fetch('/api');let d=await r.json();$('firmwareVersion').textContent=d.firmware||'—';$('state').textContent=d.state;$('dist').textContent=d.cm<0?'—':(d.cm/100).toFixed(2)+' м';$('energy').textContent=d.energy<0?'—':d.energy+'%';$('alarm').textContent=d.alarm?'ТРЕВОГА':'Нет';$('alarm').style.color=d.alarm?'#ff7b72':'#7cff9b';$('sensor').textContent=d.sensor;lastTargets=d.targets||[];drawZoneMap(lastTargets);drawWebRadar(lastTargets);$('counts').textContent=d.out+' / '+d.frames+' / '+d.bad;$('sta').textContent=d.sta||'не подключено';draw(d.history,d.tracks,d.sensor);if(d.ui)drawUi(d.ui);$('log').innerHTML=d.log.slice().reverse().map(v=>`<tr><td>${v.sec}</td><td>${v.cm<0?'—':(v.cm/100).toFixed(2)}</td><td>${v.state}</td><td>${v.out}</td><td>${v.energy<0?'—':v.energy}</td><td style="color:${v.alarm?'#ff7b72':'#7cff9b'}">${v.alarm?'ДА':'нет'}</td></tr>`).join('')}catch(e){$('state').textContent='Нет связи'}}refresh();setInterval(()=>refresh(),1000);function animateWebRadar(){drawWebRadar(lastTargets);requestAnimationFrame(animateWebRadar)}requestAnimationFrame(animateWebRadar);
</script></body></html>)HTML";

void RadarWeb::loadWifi(){wifiPrefs_.begin("radarwifi", true); ssid_=wifiPrefs_.getString("ssid", ""); password_=wifiPrefs_.getString("pass", ""); wifiPrefs_.end();}
void RadarWeb::connectSta(){ if(ssid_.length()){ WiFi.begin(ssid_.c_str(), password_.c_str()); Serial.printf("[WiFi] Подключение к домашней сети %s; точка доступа остаётся активной\n", ssid_.c_str()); } }
void RadarWeb::begin(){
  for(int t=0;t<3;t++)for(int i=0;i<80;i++)targetCm_[t][i]=-1;
  WiFi.mode(WIFI_AP_STA);
  if (strlen(WIFI_AP_PASSWORD) >= 8) WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD); else WiFi.softAP(WIFI_AP_SSID);
  loadWifi(); connectSta();
  server_.on("/", HTTP_GET, [this](){handleRoot();});
  server_.on("/api", HTTP_GET, [this](){handleApi();});
  server_.on("/screen", HTTP_GET, [this](){handleScreen();});
  server_.on("/log.csv", HTTP_GET, [this](){handleCsv();});
  server_.on("/clear", HTTP_POST, [this](){head_=0;count_=0;lastSample_=0;for(int t=0;t<3;t++)for(int i=0;i<80;i++)targetCm_[t][i]=-1;server_.send(200,"text/plain; charset=utf-8","Журнал очищен");});
  server_.on("/wifi", HTTP_POST, [this](){handleWifi();});
  server_.on("/scan", HTTP_GET, [this](){handleWifiScan();});
  server_.on("/action", HTTP_POST, [this](){handleAction();});
  server_.onNotFound([this](){server_.send(404,"text/plain","Страница не найдена");});
  server_.begin();
  Serial.printf("[WiFi] AP %s (%s) at http://%s/\n", WIFI_AP_SSID, strlen(WIFI_AP_PASSWORD) >= 8 ? "password set" : "open", WiFi.softAPIP().toString().c_str());
}
void RadarWeb::loop(){server_.handleClient();}
// POST /action?n=screen|range|bright|theme|alert|trails|demo&v=number
void RadarWeb::handleAction(){
  String name=server_.arg("n"); int value=server_.arg("v").toInt();
  bool valid=false;
  if(name=="screen") valid=value>=0&&value<7;
  else if(name=="range") valid=value>=0&&value<7;
  else if(name=="bright") valid=value>=0&&value<5;
  else if(name=="theme") valid=value>=0&&value<3;
  else if(name=="trails"||name=="demo"||name=="buzzer"||name=="sweep"||name=="rearfilter"||name=="webtrails") valid=value==0||value==1;
  else if(name=="webscale") valid=value>=0&&value<3;
  else if(name=="alert") valid=value>=0&&value<4;
  else if(name=="zones") valid=value>=0&&value<16;
  else if(name=="minrange"||name=="maxrange") valid=value>=10&&value<=2000;
  if(!valid){server_.send(400,"text/plain","bad action");return;}
  if (name == "webscale") {
    static const uint8_t sharedRangeIndex[3] = {3, 5, 6};
    pendingAction_.name = "range";
    pendingAction_.value = sharedRangeIndex[value];
  } else {
    pendingAction_.name = name;
    pendingAction_.value = value;
  }
  pendingAction_.valid = true;
  server_.send(200,"text/plain","ok");
}
bool RadarWeb::takeAction(Action& out){
  if(!pendingAction_.valid)return false;
  out=pendingAction_; pendingAction_.valid=false; return true;
}
void RadarWeb::sample(uint32_t now,const WebSample& s,int outLevel){
  for(int i=0;i<3;i++){
    const bool keepTarget = s.targetValid[i] && (ui_.rearFilterBypass || s.targetY[i] >= 0);
    targetValid_[i]=keepTarget; targetX_[i]=s.targetX[i]; targetY_[i]=s.targetY[i];
    targetTrailCount_[i]=0;
    for(uint8_t k=0;k<s.targetTrailCount[i] && k<TRAIL_LEN;k++){
      if(!ui_.rearFilterBypass && s.targetTrailY[i][k] < 0) continue;
      targetTrailX_[i][targetTrailCount_[i]]=s.targetTrailX[i][k];
      targetTrailY_[i][targetTrailCount_[i]]=s.targetTrailY[i][k];
      targetTrailCount_[i]++;
    }
  }
  frames_=s.frames; bad_=s.bad; sensor_=s.sensor;
  staStatus_ = WiFi.status()==WL_CONNECTED ? (WiFi.SSID() + String(" · ") + WiFi.localIP().toString()) : String("не подключено");
  if(now-lastSample_<1000 && count_)return; lastSample_=now;
  const int16_t loggedCm = (int16_t)(s.linkOk ? s.cm : -1);
  const uint16_t alertCm = (ui_.alertIdx == 1) ? 100 : (ui_.alertIdx == 2) ? 200 : (ui_.alertIdx == 3) ? 300 : 0;
  uint8_t alarm = (alertCm > 0 && loggedCm >= 0 && loggedCm < alertCm) ? 1 : 0;
  if (s.sensor == 2) { alarm = 0; for (int i=0;i<3;i++) if (s.targetValid[i]) { float ax=fabsf((float)s.targetX[i]), ay=fabsf((float)s.targetY[i]); uint8_t z=(s.targetY[i]<0)?3:(ax<=ay*0.50f?0:(s.targetX[i]<0?1:2)); float d=sqrtf((float)s.targetX[i]*s.targetX[i]+(float)s.targetY[i]*s.targetY[i]); if ((ui_.zoneMask&(1u<<z)) && alertCm>0 && d<alertCm*10.0f) alarm=1; } }
  rows_[head_]={now/1000,loggedCm,s.state,(uint8_t)(outLevel?1:0),s.energy,alarm};
  for(int i=0;i<3;i++) targetCm_[i][head_] = (s.sensor==2 && s.targetValid[i] && (ui_.rearFilterBypass || s.targetY[i] >= 0)) ? s.targetCm[i] : -1;
  head_=(head_+1)%80; if(count_<80)count_++;
}
void RadarWeb::handleRoot(){server_.send_P(200,"text/html; charset=utf-8",PAGE);}
void RadarWeb::handleScreen(){
  if (!screenPixels_ || screenWidth_ == 0 || screenHeight_ == 0) { server_.send(503,"text/plain","Display framebuffer unavailable"); return; }
  const size_t bytes = (size_t)screenWidth_ * screenHeight_ * sizeof(uint16_t);
  server_.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server_.setContentLength(bytes);
  server_.send(200, "application/octet-stream", "");
  server_.sendContent((const char*)screenPixels_, bytes);
}
void RadarWeb::handleApi(){
  String j; j.reserve(12000);
  const Row* latest = count_ ? &rows_[(head_ + 79) % 80] : nullptr;
  const int start = (head_ - count_ + 80) % 80;
  const int n = count_ < 60 ? count_ : 60;
  const int skip = count_ - n;
  j = "{\"firmware\":\""; j += FIRMWARE_VERSION;
  j += "\",\"state\":\"";
  j += latest ? (latest->cm < 0 ? "НЕТ ДАННЫХ" : (latest->state == 0 ? "НЕТ ЦЕЛИ" : latest->state == 1 ? "ДВИЖЕНИЕ" : latest->state == 2 ? "НЕПОДВИЖНО" : "ДВИЖ.+СТАТ.")) : "НЕТ ДАННЫХ";
  j += "\",\"cm\":"; j += latest ? String(latest->cm) : String(-1);
  j += ",\"energy\":"; j += latest ? String(latest->energy) : String(-1);
  j += ",\"alarm\":"; j += latest ? String(latest->alarm) : String(0);
  j += ",\"sensor\":\""; j += (sensor_ == 2 ? "LD2450" : sensor_ == 1 ? "LD2401" : "-"); j += "\"";
  j += ",\"out\":"; j += latest ? String(latest->out) : String(0);
  j += ",\"frames\":"; j += String(frames_);
  j += ",\"bad\":"; j += String(bad_);
  j += ",\"sta\":\""; String sta = staStatus_; sta.replace("\\", "\\\\"); sta.replace("\"", "\\\""); j += sta; j += "\"";
  j += ",\"history\":[";
  for (int i = 0; i < n; ++i) {
    if (i) j += ",";
    const Row& r = rows_[(start + skip + i) % 80];
    j += "{\"cm\":"; j += String(r.cm); j += ",\"sec\":"; j += String(r.ms); j += "}";
  }
  j += "],\"tracks\":[";
  for (int t = 0; t < 3; ++t) {
    if (t) j += ","; j += "[";
    for (int i = 0; i < n; ++i) { if (i) j += ","; j += String(targetCm_[t][(start + skip + i) % 80]); }
    j += "]";
  }
  j += "],\"log\":[";
  for (int i = 0; i < count_; ++i) {
    if (i) j += ",";
    const Row& r = rows_[(start + i) % 80];
    j += "{\"sec\":"; j += String(r.ms); j += ",\"cm\":"; j += String(r.cm);
    j += ",\"state\":\""; j += (r.cm < 0 ? "НЕТ ДАННЫХ" : r.state == 0 ? "НЕТ ЦЕЛИ" : r.state == 1 ? "ДВИЖЕНИЕ" : r.state == 2 ? "НЕПОДВИЖНО" : "ДВИЖ.+СТАТ.");
    j += "\",\"out\":"; j += String(r.out); j += ",\"energy\":"; j += String(r.energy); j += ",\"alarm\":"; j += String(r.alarm); j += "}";
  }
  j += "],\"ui\":{\"screen\":"; j += String(ui_.screen);
  j += ",\"item\":"; j += String(ui_.item); j += ",\"range\":"; j += String(ui_.rangeIdx);
  j += ",\"bright\":"; j += String(ui_.brightIdx); j += ",\"theme\":"; j += String(ui_.theme);
  j += ",\"alert\":"; j += String(ui_.alertIdx);
  j += ",\"trails\":"; j += String(ui_.trails ? 1 : 0); j += ",\"demo\":"; j += String(ui_.demo ? 1 : 0);
  j += ",\"buzzer\":"; j += String(ui_.buzzer ? 1 : 0); j += ",\"sweep\":"; j += String(ui_.sweep ? 1 : 0);
  j += ",\"zones\":"; j += String(ui_.zoneMask);
  j += ",\"rearFilterBypass\":"; j += String(ui_.rearFilterBypass ? 1 : 0);
  j += ",\"webTrails\":"; j += String(ui_.webTrails ? 1 : 0);
  j += ",\"webScaleIdx\":"; j += String(ui_.webScaleIdx); j += ",\"minCm\":"; j += String(ui_.minRangeCm);
  j += ",\"maxCm\":"; j += String(ui_.maxRangeCm); j += "},\"targets\":[";
  for (int i = 0; i < 3; ++i) {
    if (i) j += ",";
    j += "{\"valid\":"; j += (targetValid_[i] ? "true" : "false");
    j += ",\"x\":"; j += String(targetX_[i]); j += ",\"y\":"; j += String(targetY_[i]);
    j += ",\"trail\":[";
    for (int k = (int)targetTrailCount_[i] - 1; k >= 0; --k) {
      if (k != (int)targetTrailCount_[i] - 1) j += ",";
      j += "["; j += String(targetTrailX_[i][k]); j += ","; j += String(targetTrailY_[i][k]); j += "]";
    }
    j += "]}";
  }
  j += "]}";
  server_.send(200, "application/json", j);
}
void RadarWeb::handleCsv(){String s="время_с,расстояние_см,состояние,out,энергия_процентов,тревога\r\n";int start=(head_-count_+80)%80;for(int i=0;i<count_;i++){const Row&r=rows_[(start+i)%80];s+=String(r.ms)+","+String(r.cm)+","+String(r.state)+","+String(r.out)+","+(r.energy<0?String(""):String(r.energy))+","+String(r.alarm)+"\r\n";}server_.sendHeader("Content-Disposition","attachment; filename=esp32-radar-zhurnal.csv");server_.send(200,"text/csv; charset=utf-8",s);}
void RadarWeb::handleWifiScan(){
  const int n=WiFi.scanNetworks(false,false);
  String json="{\"networks\":[";
  for(int i=0;i<n;i++){
    if(i)json+=",";
    String name=WiFi.SSID(i);
    name.replace("\\","\\\\"); name.replace("\"","\\\"");
    json+="{\"ssid\":\""+name+"\",\"rssi\":"+String(WiFi.RSSI(i))+",\"secure\":";
    json+=(WiFi.encryptionType(i)==WIFI_AUTH_OPEN?"false":"true"); json+="}";
  }
  json+="]}"; WiFi.scanDelete();
  server_.sendHeader("Cache-Control","no-store");
  server_.send(200,"application/json; charset=utf-8",json);
}
void RadarWeb::handleWifi(){ssid_=server_.arg("ssid");password_=server_.arg("password");ssid_.trim();if(ssid_.length()>0){wifiPrefs_.begin("radarwifi",false);wifiPrefs_.putString("ssid",ssid_);wifiPrefs_.putString("pass",password_);wifiPrefs_.end();WiFi.mode(WIFI_AP_STA);connectSta();}server_.sendHeader("Location","/");server_.send(303,"text/plain","Сохранено. Точка доступа остаётся активной.");}
