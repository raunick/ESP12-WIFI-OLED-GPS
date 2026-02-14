#pragma once

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>RoboEyes Dashboard v5.1</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,sans-serif;background:#0a0a0f;color:#e0e0e0;min-height:100vh;padding:16px}
h1{text-align:center;font-size:1.6rem;margin:12px 0;background:linear-gradient(135deg,#00d4ff,#7b2fff);-webkit-background-clip:text;-webkit-text-fill-color:transparent}
.subtitle{text-align:center;font-size:.8rem;color:#666;margin-bottom:8px}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:14px;max-width:900px;margin:0 auto}
.card{background:rgba(255,255,255,.04);border:1px solid rgba(255,255,255,.08);border-radius:16px;padding:18px;backdrop-filter:blur(12px)}
.card h2{font-size:.85rem;color:#888;text-transform:uppercase;letter-spacing:1px;margin-bottom:14px;display:flex;align-items:center;gap:8px}
.card h2 span{font-size:1.1rem}
.sensor-row{display:flex;justify-content:space-between;align-items:center;padding:10px 0;border-bottom:1px solid rgba(255,255,255,.05)}
.sensor-row:last-child{border:none}
.sensor-label{font-size:.85rem;color:#aaa}
.sensor-value{font-size:1.3rem;font-weight:700;font-variant-numeric:tabular-nums}
.val-temp{color:#ff6b6b}.val-hum{color:#51cf66}.val-accel{color:#339af0}
.status{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:6px}
.status.on{background:#51cf66;box-shadow:0 0 8px #51cf66}.status.off{background:#555}
.toggle-row{display:flex;justify-content:space-between;align-items:center;padding:8px 0;border-bottom:1px solid rgba(255,255,255,.05)}
.toggle-row:last-child{border:none}
.toggle-name{font-size:.85rem}
.switch{position:relative;width:44px;height:24px;cursor:pointer}
.switch input{opacity:0;width:0;height:0}
.slider{position:absolute;top:0;left:0;right:0;bottom:0;background:#333;border-radius:24px;transition:.3s}
.slider:before{content:'';position:absolute;height:18px;width:18px;left:3px;bottom:3px;background:#888;border-radius:50%;transition:.3s}
input:checked+.slider{background:linear-gradient(135deg,#00d4ff,#7b2fff)}
input:checked+.slider:before{transform:translateX(20px);background:#fff}
.mood-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:8px}
.mood-btn{padding:10px;border:2px solid rgba(255,255,255,.1);border-radius:12px;background:rgba(255,255,255,.03);color:#ccc;font-size:.8rem;font-weight:600;cursor:pointer;transition:.2s;text-align:center}
.mood-btn:hover{border-color:rgba(255,255,255,.3);background:rgba(255,255,255,.08)}
.action-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}
.action-btn{padding:12px 8px;border:1px solid rgba(255,255,255,.1);border-radius:12px;background:rgba(255,255,255,.03);color:#ccc;font-size:.75rem;font-weight:600;cursor:pointer;transition:.2s;text-align:center}
.action-btn:hover{background:rgba(0,212,255,.15);border-color:#00d4ff;color:#fff;transform:scale(1.03)}
.action-btn:active{transform:scale(.97)}
.action-btn.new{border-color:rgba(123,47,255,.3);background:rgba(123,47,255,.08)}
.action-btn.new:hover{border-color:#7b2fff;background:rgba(123,47,255,.2)}
.menu-btn{width:100%;padding:14px;border:2px solid rgba(0,212,255,.3);border-radius:12px;background:rgba(0,212,255,.08);color:#00d4ff;font-size:.9rem;font-weight:700;cursor:pointer;transition:.2s;text-align:center;margin-top:8px;letter-spacing:.5px}
.menu-btn:hover{background:rgba(0,212,255,.2);border-color:#00d4ff;transform:scale(1.02)}
.menu-btn:active{transform:scale(.98)}
.cal-row{margin:12px 0}
.cal-label{display:flex;justify-content:space-between;font-size:.8rem;color:#aaa;margin-bottom:6px}
input[type=range]{width:100%;height:6px;border-radius:3px;background:#222;outline:none;-webkit-appearance:none;appearance:none}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:18px;height:18px;border-radius:50%;background:linear-gradient(135deg,#00d4ff,#7b2fff);cursor:pointer}
.shape-row{display:flex;justify-content:space-between;align-items:center;padding:6px 0}
.shape-label{font-size:.8rem;color:#aaa}
.shape-input{width:60px;padding:4px 8px;background:#1a1a24;border:1px solid rgba(255,255,255,.15);border-radius:8px;color:#fff;font-size:.85rem;text-align:center}
.footer{text-align:center;margin-top:20px;font-size:.7rem;color:#444}
.mode-badge{display:inline-block;padding:3px 10px;border-radius:20px;font-size:.7rem;font-weight:700;margin-left:8px}
.mode-eyes{background:rgba(81,207,102,.15);color:#51cf66}
.mode-menu{background:rgba(0,212,255,.15);color:#00d4ff}
.mode-anim{background:rgba(123,47,255,.15);color:#7b2fff}
.info-bar{display:flex;justify-content:center;gap:16px;font-size:.75rem;color:#555;margin-bottom:16px;padding:8px;background:rgba(255,255,255,.02);border-radius:8px;flex-wrap:wrap}
.info-bar .item{display:flex;align-items:center;gap:4px}
.clock-display{font-size:1.5rem;font-weight:700;text-align:center;color:#00d4ff;margin:10px 0;font-variant-numeric:tabular-nums}
.clock-date{text-align:center;font-size:.85rem;color:#666}
.splash-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-top:12px}
.splash-btn{padding:10px 6px;border:2px solid rgba(255,255,255,.1);border-radius:12px;background:rgba(255,255,255,.03);color:#aaa;font-size:.75rem;font-weight:600;cursor:pointer;transition:.2s;text-align:center}
.splash-btn:hover{border-color:rgba(0,212,255,.3);color:#fff}
.splash-btn.active{border-color:#00d4ff;background:rgba(0,212,255,.12);color:#00d4ff}
.touch-val{font-size:1.8rem;font-weight:700;text-align:center;color:#7b2fff;margin:8px 0}
.mem-bar{height:6px;border-radius:3px;background:#222;margin-top:4px;overflow:hidden}
.mem-fill{height:100%;border-radius:3px;background:linear-gradient(90deg,#51cf66,#00d4ff);transition:width .5s}
</style>
</head>
<body>
<h1>🤖 RoboEyes Dashboard</h1>
<p class="subtitle">ESP32-WROOM • v5.1 <span class="mode-badge mode-eyes" id="mode-badge">OLHOS</span></p>

<div class="info-bar">
  <div class="item">📡 WiFi: RoboEyes</div>
  <div class="item">🔑 roboeyes123</div>
  <div class="item">🌐 192.168.4.1</div>
  <div class="item" id="clock-bar">⏰ --:--:--</div>
</div>

<div class="grid">

  <!-- SENSORES -->
  <div class="card">
    <h2><span>📊</span> Sensores</h2>
    <div class="sensor-row">
      <span class="sensor-label"><span class="status" id="htu-st"></span>Temperatura</span>
      <span class="sensor-value val-temp" id="temp">--°C</span>
    </div>
    <div class="sensor-row">
      <span class="sensor-label">Umidade</span>
      <span class="sensor-value val-hum" id="hum">--%</span>
    </div>
    <div class="sensor-row">
      <span class="sensor-label"><span class="status" id="bmi-st"></span>Accel X</span>
      <span class="sensor-value val-accel" id="ax">--</span>
    </div>
    <div class="sensor-row">
      <span class="sensor-label">Accel Y / Z</span>
      <span class="sensor-value val-accel" id="ayz">-- / --</span>
    </div>
    <div class="sensor-row">
      <span class="sensor-label">Humor</span>
      <span class="sensor-value" id="mood" style="color:#7b2fff">--</span>
    </div>
    <div class="sensor-row">
      <span class="sensor-label">👆 Touch (GPIO15)</span>
      <span class="sensor-value" id="touchval" style="color:#ff922b">--</span>
    </div>
    <div class="sensor-row">
      <span class="sensor-label">💾 Heap Livre</span>
      <span class="sensor-value" id="heap" style="color:#51cf66;font-size:1rem">-- KB</span>
    </div>
  </div>

  <!-- TOGGLES -->
  <div class="card">
    <h2><span>🎛️</span> Features</h2>
    <div class="toggle-row"><span class="toggle-name">👀 Eye Tracking</span><label class="switch"><input type="checkbox" id="t-tracking" onchange="toggle('tracking',this.checked)"><span class="slider"></span></label></div>
    <div class="toggle-row"><span class="toggle-name">🌡️ Auto Mood</span><label class="switch"><input type="checkbox" id="t-automood" onchange="toggle('automood',this.checked)"><span class="slider"></span></label></div>
    <div class="toggle-row"><span class="toggle-name">🔊 Buzzer</span><label class="switch"><input type="checkbox" id="t-buzzer" onchange="toggle('buzzer',this.checked)"><span class="slider"></span></label></div>
    <div class="toggle-row"><span class="toggle-name">💡 LED RGB</span><label class="switch"><input type="checkbox" id="t-led" onchange="toggle('led',this.checked)"><span class="slider"></span></label></div>
    <div class="toggle-row"><span class="toggle-name">😉 Auto Blinker</span><label class="switch"><input type="checkbox" id="t-blinker" onchange="toggle('blinker',this.checked)"><span class="slider"></span></label></div>
    <div class="toggle-row"><span class="toggle-name">🔄 Idle Mode</span><label class="switch"><input type="checkbox" id="t-idle" onchange="toggle('idle',this.checked)"><span class="slider"></span></label></div>
    <div class="toggle-row"><span class="toggle-name">💧 Sweat</span><label class="switch"><input type="checkbox" id="t-sweat" onchange="toggle('sweat',this.checked)"><span class="slider"></span></label></div>
    <div class="toggle-row"><span class="toggle-name">🧐 Curiosity</span><label class="switch"><input type="checkbox" id="t-curiosity" onchange="toggle('curiosity',this.checked)"><span class="slider"></span></label></div>
    <div class="toggle-row" style="border-top:1px solid rgba(123,47,255,.2);padding-top:12px"><span class="toggle-name">🎭 Auto Expressões</span><label class="switch"><input type="checkbox" id="t-autoExpr" onchange="toggle('autoExpr',this.checked)"><span class="slider"></span></label></div>
    <div class="toggle-row"><span class="toggle-name">👆 Touch Sensor</span><label class="switch"><input type="checkbox" id="t-touch" onchange="toggle('touch',this.checked)"><span class="slider"></span></label></div>
    <div class="toggle-row" style="border-top:1px solid rgba(255,255,255,.1);padding-top:12px"><span class="toggle-name">🔲 Inversão OLED</span><label class="switch"><input type="checkbox" id="t-invert" onchange="toggle('invert',this.checked)"><span class="slider"></span></label></div>
    <button class="menu-btn" onclick="showOledMenu()">📺 Mostrar Info no OLED</button>
  </div>

  <!-- EXPRESSÕES -->
  <div class="card">
    <h2><span>😄</span> Expressão</h2>
    <div class="mood-grid">
      <button class="mood-btn" onclick="setMood('happy')">😊 Happy</button>
      <button class="mood-btn" onclick="setMood('angry')">😠 Angry</button>
      <button class="mood-btn" onclick="setMood('tired')">😴 Tired</button>
      <button class="mood-btn" onclick="setMood('default')">😐 Default</button>
    </div>
    <h2 style="margin-top:16px"><span>🎬</span> Animações</h2>
    <div class="action-grid">
      <button class="action-btn" onclick="doAction('blink')">😉 Piscar</button>
      <button class="action-btn" onclick="doAction('confused')">😵 Confuso</button>
      <button class="action-btn" onclick="doAction('laugh')">😂 Rir</button>
      <button class="action-btn" onclick="doAction('wink_l')">🫣 Piscar E</button>
      <button class="action-btn" onclick="doAction('wink_r')">😜 Piscar D</button>
      <button class="action-btn" onclick="doAction('cyclops')">👁️ Ciclope</button>
    </div>
    <h2 style="margin-top:16px"><span>🎭</span> Expressões Especiais</h2>
    <div class="action-grid">
      <button class="action-btn new" onclick="doAction('love')">😍 Love</button>
      <button class="action-btn new" onclick="doAction('scared')">😱 Scared</button>
      <button class="action-btn new" onclick="doAction('suspicious')">🤨 Suspicious</button>
      <button class="action-btn new" onclick="doAction('sleepy')">😪 Sleepy</button>
      <button class="action-btn new" onclick="doAction('excited')">🤩 Excited</button>
      <button class="action-btn new" onclick="doAction('dizzy')">😵‍💫 Dizzy</button>
    </div>
  </div>

  <!-- CALIBRAÇÃO + SPLASH -->
  <div class="card">
    <h2><span>⚙️</span> Calibração</h2>
    <div class="cal-row">
      <div class="cal-label"><span>Sensibilidade Olhos</span><span id="th-val">0.30</span></div>
      <input type="range" id="threshold" min="0.1" max="0.8" step="0.05" value="0.3" oninput="document.getElementById('th-val').textContent=parseFloat(this.value).toFixed(2)" onchange="calibrate()">
    </div>
    <div class="cal-row">
      <div class="cal-label"><span>Sensibilidade Sacudida</span><span id="sh-val">1.50</span></div>
      <input type="range" id="shake" min="0.5" max="3.0" step="0.1" value="1.5" oninput="document.getElementById('sh-val').textContent=parseFloat(this.value).toFixed(2)" onchange="calibrate()">
    </div>
    <h2 style="margin-top:16px"><span>📐</span> Forma dos Olhos</h2>
    <div class="shape-row"><span class="shape-label">Largura</span><input type="number" class="shape-input" id="eye-w" value="36" min="10" max="60" onchange="setShape()"></div>
    <div class="shape-row"><span class="shape-label">Altura</span><input type="number" class="shape-input" id="eye-h" value="36" min="10" max="60" onchange="setShape()"></div>
    <div class="shape-row"><span class="shape-label">Borda</span><input type="number" class="shape-input" id="eye-r" value="8" min="0" max="30" onchange="setShape()"></div>
    <div class="shape-row"><span class="shape-label">Espaço</span><input type="number" class="shape-input" id="eye-s" value="10" min="-20" max="40" onchange="setShape()"></div>

    <h2 style="margin-top:16px"><span>🖼️</span> Tema de Intro</h2>
    <div class="splash-grid">
      <button class="splash-btn active" id="sp-0" onclick="setSplash(0)">✨ Minimal</button>
      <button class="splash-btn" id="sp-1" onclick="setSplash(1)">💚 Matrix</button>
      <button class="splash-btn" id="sp-2" onclick="setSplash(2)">🌊 Wave</button>
    </div>
  </div>

</div>

<p class="footer">RoboEyes Enhanced v5.1 • Touch + Mochi + NVS</p>

<script>
const API='';
let currentSplash=0;

async function fetchStatus(){
  try{
    const r=await fetch(API+'/api/status');
    const d=await r.json();
    document.getElementById('temp').textContent=d.temp.toFixed(1)+'°C';
    document.getElementById('hum').textContent=d.hum.toFixed(0)+'%';
    document.getElementById('ax').textContent=d.ax.toFixed(2);
    document.getElementById('ayz').textContent=d.ay.toFixed(2)+' / '+d.az.toFixed(2);
    const moods=['DEFAULT','HAPPY','TIRED','ANGRY'];
    document.getElementById('mood').textContent=moods[d.mood]||'DEFAULT';
    document.getElementById('htu-st').className='status '+(d.htu?'on':'off');
    document.getElementById('bmi-st').className='status '+(d.bmi?'on':'off');
    document.getElementById('touchval').textContent=d.touch;
    document.getElementById('t-tracking').checked=d.t.tracking;
    document.getElementById('t-automood').checked=d.t.automood;
    document.getElementById('t-buzzer').checked=d.t.buzzer;
    document.getElementById('t-led').checked=d.t.led;
    document.getElementById('t-blinker').checked=d.t.blinker;
    document.getElementById('t-idle').checked=d.t.idle;
    document.getElementById('t-sweat').checked=d.t.sweat;
    document.getElementById('t-curiosity').checked=d.t.curiosity;
    document.getElementById('t-autoExpr').checked=d.t.autoExpr;
    document.getElementById('t-touch').checked=d.t.touch;
    document.getElementById('t-invert').checked=d.t.invert;
    if(d.heap) document.getElementById('heap').textContent=(d.heap/1024).toFixed(1)+' KB';
    const badge=document.getElementById('mode-badge');
    if(d.mode===0){badge.textContent='OLHOS';badge.className='mode-badge mode-eyes';}
    else if(d.mode===1){badge.textContent='MENU';badge.className='mode-badge mode-menu';}
    else{badge.textContent='ANIMAÇÃO';badge.className='mode-badge mode-anim';}
    if(d.clockSynced){
      document.getElementById('clock-bar').textContent='⏰ '+String(d.ch).padStart(2,'0')+':'+String(d.cm).padStart(2,'0')+':'+String(d.cs).padStart(2,'0');
    }
  }catch(e){console.error(e);}
}

async function syncTime(){
  const n=new Date();
  await fetch(API+'/api/time',{method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({h:n.getHours(),m:n.getMinutes(),s:n.getSeconds(),d:n.getDate(),mo:n.getMonth()+1,y:n.getFullYear()})});
}
async function toggle(f,s){await fetch(API+'/api/toggle',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({feature:f,state:s})});}
async function setMood(m){await fetch(API+'/api/mood',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({mood:m})});}
async function doAction(a){await fetch(API+'/api/eyes',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({action:a})});}
async function calibrate(){
  await fetch(API+'/api/calibrate',{method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({threshold:parseFloat(document.getElementById('threshold').value),shakeThreshold:parseFloat(document.getElementById('shake').value)})});
}
async function setShape(){
  await fetch(API+'/api/shape',{method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({w:+document.getElementById('eye-w').value,h:+document.getElementById('eye-h').value,r:+document.getElementById('eye-r').value,s:+document.getElementById('eye-s').value})});
}
async function showOledMenu(){await fetch(API+'/api/screen',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({action:'menu'})});}
async function setSplash(t){
  currentSplash=t;
  document.querySelectorAll('.splash-btn').forEach((b,i)=>b.classList.toggle('active',i===t));
  await fetch(API+'/api/splash',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({theme:t})});
}

// Auto sync time every 30s + on load
syncTime();
setInterval(syncTime,30000);
setInterval(fetchStatus,1000);
fetchStatus();
</script>
</body>
</html>
)rawliteral";
