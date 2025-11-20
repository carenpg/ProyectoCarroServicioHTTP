// ====== Config MQTT ======
const NS = 'sebas/esp32car';
const TOPIC_CMD      = `${NS}/cmd`;
const TOPIC_STATUS   = `${NS}/status`;
const TOPIC_DISTANCE = `${NS}/distance`;
const TOPIC_IMU      = `${NS}/imu`;
const TOPIC_ALERT    = `${NS}/alert`;
const TOPIC_LWT      = `${NS}/lwt`;

const clientId = `webdash-${Math.random().toString(16).slice(2,8)}`;
document.getElementById('clientId').textContent = clientId;

// Si sirves por HTTPS, cambia a wss://broker.hivemq.com:8884/mqtt
const url = 'ws://broker.hivemq.com:8000/mqtt';
const client = mqtt.connect(url, {
  clientId,
  keepalive: 30,
  clean: true,
  reconnectPeriod: 2000
});

// ====== UI helpers ======
const el = id => document.getElementById(id);
const log = (m) => {
  const p = el('log');
  const ts = new Date().toLocaleTimeString();
  p.textContent = `[${ts}] ${m}\n` + p.textContent;
};

// Toast
let toastTimer = null;
function showToast(text, color = '#1f6feb'){
  const t = el('toast');
  t.textContent = text;
  t.style.background = color;
  t.classList.add('show');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(()=> t.classList.remove('show'), 1600);
}

// ====== Estado de confirmación de comandos ======
let pendingCmd = null; // {dir, speed, deadline}
function setPending(cmd){
  pendingCmd = cmd;
  el('lastCmd').textContent = JSON.stringify(cmd);
  el('lastAck').textContent = 'pendiente';
  el('lastAck').className = 'badge gray';
}
function setAck(ok, reason=''){
  if (ok){
    el('lastAck').textContent = 'confirmado';
    el('lastAck').className = 'badge';
  } else {
    el('lastAck').textContent = reason || 'rechazado';
    el('lastAck').className = 'badge red';
  }
  pendingCmd = null;
}

// ====== Conexión ======
client.on('connect', () => {
  el('conn').textContent = 'conectado';
  el('conn').className = 'badge';
  client.subscribe(`${NS}/#`, err => {
    if (err) log('Error suscribiendo: ' + err.message);
  });
  log('MQTT conectado');
});
client.on('reconnect', () => { el('conn').textContent = 'reconectando…'; el('conn').className = 'badge gray'; });
client.on('close', () => { el('conn').textContent = 'desconectado'; el('conn').className = 'badge red'; });
client.on('error', (e) => { log('MQTT error: ' + e.message); });

// ====== Chart aceleración (EJE Y FIJO) ======
const ctx = document.getElementById('accChart').getContext('2d');
const accData = {
  labels: [],
  datasets: [
    { label: 'ax', data: [], borderWidth: 1, tension: .15 },
    { label: 'ay', data: [], borderWidth: 1, tension: .15 },
    { label: 'az', data: [], borderWidth: 1, tension: .15 },
  ]
};
const accChart = new Chart(ctx, {
  type: 'line',
  data: accData,
  options: {
    animation: false,
    responsive: true,
    maintainAspectRatio: false,
    scales: {
      x: { display:false },
      y: {
        min: -5,                  // límites fijos del eje Y
        max:  5,
        ticks:{ color:'#e6e6e6' }
      }
    },
    plugins: { legend:{ labels:{ color:'#e6e6e6' } } }
  }
});
function pushAcc(ax, ay, az) {
  const maxPoints = 120;
  accData.labels.push('');
  accData.datasets[0].data.push(ax);
  accData.datasets[1].data.push(ay);
  accData.datasets[2].data.push(az);
  if (accData.labels.length > maxPoints){
    accData.labels.shift();
    accData.datasets.forEach(d => d.data.shift());
  }
  accChart.update('none');
}

// ====== Handlers de mensajes ======
client.on('message', (topic, payload) => {
  let msg;
  try { msg = JSON.parse(payload.toString()); } catch { msg = payload.toString(); }

  if (topic === TOPIC_STATUS) {
    el('statusText').textContent = (typeof msg==='object') ? JSON.stringify(msg) : String(msg);

    // Confirmación por coincidencia de estado
    if (pendingCmd && typeof msg === 'object'){
      if (pendingCmd.dir === 'stop'){
        if (msg.moving === false) setAck(true);
      } else if (['forward','backward','left','right'].includes(pendingCmd.dir)) {
        if (msg.dir === pendingCmd.dir && msg.moving === true) setAck(true);
      }
    }
  }
  else if (topic === TOPIC_DISTANCE) {
    if (typeof msg === 'object') {
      const cm = msg.cm, obst = !!msg.obstacle;
      el('distance').textContent = (cm>=0) ? cm.toFixed(1) : '—';
      el('distanceRaw').textContent = JSON.stringify(msg);
      const badge = el('obstacleBadge');
      if (obst) {
        badge.textContent = 'OBSTÁCULO';
        badge.className = 'badge red';
        log('⚠️ Obstáculo detectado, se envía STOP');
        showToast('⚠️ Obstáculo detectado', '#d73a49');
        alert('🚨 ¡Obstáculo detectado! El coche se detuvo automáticamente.');
        sendCmd({stop:true});   // auto-stop
      } else {
        badge.textContent = 'OK';
        badge.className = 'badge';
      }
    } else {
      el('distanceRaw').textContent = msg;
    }
  }
  else if (topic === TOPIC_IMU) {
    if (typeof msg === 'object') {
      el('imuRaw').textContent = `ax:${msg.ax?.toFixed(2)} ay:${msg.ay?.toFixed(2)} az:${msg.az?.toFixed(2)} | gx:${msg.gx?.toFixed(1)} gy:${msg.gy?.toFixed(1)} gz:${msg.gz?.toFixed(1)}`;
      pushAcc(msg.ax, msg.ay, msg.az);
    } else {
      el('imuRaw').textContent = msg;
    }
  }
  else if (topic === TOPIC_ALERT) {
    el('alertText').textContent = JSON.stringify(msg);
    if (pendingCmd && pendingCmd.dir === 'forward'){
      setAck(false, 'obstáculo');
    }
    log('ALERTA: ' + JSON.stringify(msg));
    showToast('⚠️ ' + JSON.stringify(msg), '#d73a49');
    alert('⚠️ ALERTA: ' + JSON.stringify(msg));
  }
  else if (topic === TOPIC_LWT) {
    log('LWT: ' + payload.toString());
  }
});

// ====== Publicar comandos + feedback visual ======
function sendCmd(obj){
  setPending({ dir: obj.stop ? 'stop' : obj.dir, speed: obj.speed ?? null, deadline: Date.now() + 3000 });
  el('lastAck').textContent = 'pendiente';
  el('lastAck').className = 'badge gray';

  client.publish(TOPIC_CMD, JSON.stringify(obj));
  const text = '→ CMD ' + JSON.stringify(obj);
  log(text);
  showToast(text);

  // Timeout de confirmación
  setTimeout(() => {
    if (pendingCmd && pendingCmd.deadline && Date.now() > pendingCmd.deadline){
      setAck(false, 'sin respuesta');
    }
  }, 3200);
}

// ====== Controles ======
const speed = el('speed'), speedLbl = el('speedLbl'), duration = el('duration');
speed.addEventListener('input', ()=> speedLbl.textContent = speed.value);

el('btnForward').onclick = () => sendCmd({dir:'forward', speed:+speed.value, duration:+duration.value});
el('btnBackward').onclick= () => sendCmd({dir:'backward',speed:+speed.value, duration:+duration.value});
el('btnLeft').onclick    = () => sendCmd({dir:'left',    speed:+speed.value, duration:+duration.value});
el('btnRight').onclick   = () => sendCmd({dir:'right',   speed:+speed.value, duration:+duration.value});
el('btnStop').onclick    = () => sendCmd({stop:true});
el('btnApplySafe').onclick = () => {
  const v = +el('safe').value;
  if (v>0 && v<2000) sendCmd({safe_stop_cm: v});
};
