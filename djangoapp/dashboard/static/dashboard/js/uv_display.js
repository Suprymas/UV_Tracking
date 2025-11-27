const CURRENT_ENDPOINT = UV_API_CURRENT_URL;
const HISTORY_ENDPOINT = UV_API_HISTORY_URL;
const POLL_INTERVAL_MS = 60000;

const uvValueEl = document.getElementById('uv-value');
const uvCategoryEl = document.getElementById('uv-category');
const uvTimestampEl = document.getElementById('uv-timestamp');
const refreshBtn = document.getElementById('refresh-button');
const autoRefreshCheckbox = document.getElementById('auto-refresh');
const measureButton = document.getElementById('uv-test-button');
const countdownDisplay = document.getElementById('countdown-display');
let uvChart = null;

// --- UV Index ---
function uvCategory(uv) {
  if (uv == null || isNaN(uv)) return {label:'Unknown', level:'unknown'};
  if (uv <= 2) return {label:'Low', level:'low'};
  if (uv <= 5) return {label:'Moderate', level:'moderate'};
  if (uv <= 7) return {label:'High', level:'high'};
  if (uv <= 10) return {label:'Very High', level:'very-high'};
  return {label:'Extreme', level:'extreme'};
}

function formatTimestamp(iso) {
  try { return new Date(iso).toLocaleString(); } catch { return iso; }
}

function updateCurrent(data) {
  const uv = (data && typeof data.uv === 'number') ? data.uv : null;
  const ts = (data && data.timestamp) ? data.timestamp : null;
  uvValueEl.textContent = uv == null ? '—' : uv.toFixed(1);
  const cat = uvCategory(uv);
  uvCategoryEl.textContent = cat.label;
  uvCategoryEl.dataset.level = cat.level;
  uvTimestampEl.textContent = ts ? formatTimestamp(ts) : '—';
}

async function fetchCurrent() {
  const res = await fetch(CURRENT_ENDPOINT, {cache:'no-cache'});
  if (!res.ok) throw new Error('Network error');
  const json = await res.json();
  updateCurrent(json);
}

async function fetchHistoryAndDraw() {
  const res = await fetch(HISTORY_ENDPOINT, {cache:'no-cache'});
  if (!res.ok) throw new Error('Network error');
  const json = await res.json();
  const labels = json.map(item => new Date(item.timestamp).toLocaleTimeString());
  const data = json.map(item => item.uv);
  const ctx = document.getElementById('uv-chart').getContext('2d');

  if (uvChart) {
    uvChart.data.labels = labels;
    uvChart.data.datasets[0].data = data;
    uvChart.update();
    return;
  }

  uvChart = new Chart(ctx, {
    type: 'line',
    data: { labels, datasets: [{ label: 'UV index', data, fill: true, tension: 0.3, pointRadius: 0, borderWidth: 2 }] },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: { x:{ ticks:{ autoSkip: true } }, y:{ beginAtZero:true, suggestedMax:12 } },
      plugins: { legend:{ display:false } },
      elements:{ line:{ borderJoinStyle:'round' } }
    }
  });
}

// --- Polling ---
let pollTimer = null;
function startPolling() { if(pollTimer) clearInterval(pollTimer); pollTimer = setInterval(()=>{ fetchCurrent(); fetchHistoryAndDraw(); }, POLL_INTERVAL_MS);}
function stopPolling() { if(pollTimer) clearInterval(pollTimer);}

refreshBtn.addEventListener('click', ()=>{ fetchCurrent(); fetchHistoryAndDraw(); });
autoRefreshCheckbox.addEventListener('change', ()=>{ autoRefreshCheckbox.checked ? startPolling() : stopPolling(); });

(async function init() {
  await fetchCurrent();
  await fetchHistoryAndDraw();
  if (autoRefreshCheckbox.checked) startPolling();
})();

// --- Skin Measurement ---
measureButton.addEventListener('click', startMeasurementSequence);

function startMeasurementSequence() {
  const display = countdownDisplay;
  const resultBox = document.getElementById("skin-analysis-result");
  resultBox.style.display = "none";
  display.innerText = "Scanning... 3";
  setTimeout(()=>display.innerText="Scanning... 2",1000);
  setTimeout(()=>display.innerText="Scanning... 1",2000);
  setTimeout(()=>{ display.innerText="Scan Complete!"; simulateSensor(200,170,140); },3000);
}

function simulateSensor(r,g,b) {
  let uvText = uvValueEl.innerText;
  let currentUV = parseFloat(uvText);
  if(isNaN(currentUV)) currentUV = 5;
  const analysis = analyzeSkin(r,g,b,currentUV);
  updateAnalysisUI(analysis);
}

function updateAnalysisUI(data){
  const resultBox = document.getElementById("skin-analysis-result");
  document.getElementById("display-type").innerText = data.classification.type + ": " + data.classification.description;
  document.getElementById("display-ita").innerText = data.colorData.ita;
  document.getElementById("rec-spf").innerText = data.recommendations.spfFactor;
  document.getElementById("rec-time").innerText = data.recommendations.reapplyFrequency;
  document.getElementById("rec-tip").innerText = data.recommendations.healthTip;
  document.getElementById("color-preview").style.backgroundColor = `rgb(${data.colorData.r},${data.colorData.g},${data.colorData.b})`;
  resultBox.style.display = "block";
  countdownDisplay.innerText = "";
}

// --- Fitzpatrick Scale via ITA ---
function analyzeSkin(r,g,b,uvIndex){
  const lab = rgbToLab(r,g,b);
  const ita = Math.atan2(lab.L-50, lab.b)*(180/Math.PI);
  let type="",desc="";
  if (ita>55) type="Type I",desc="Very Light";
  else if(ita>41) type="Type II",desc="Light";
  else if(ita>28) type="Type III",desc="Medium";
  else if(ita>10) type="Type IV",desc="Tan";
  else if(ita>-30) type="Type V",desc="Brown";
  else type="Type VI",desc="Dark";

  let spf="SPF 30",time="Every 120 min",tip="";
  if(type==="Type I"||type==="Type II"){
    spf="SPF 50+";
    tip="High burn risk. Wear hat & sunglasses.";
    time=uvIndex>=6 ? "Every 30-60 min (High Risk!)" : "Every 45-60 min";
  } else if(type==="Type III"||type==="Type IV"){
    spf="SPF 30-50";
    tip="Moderate burn risk. Seek shade at noon.";
    time=uvIndex>=8 ? "Every 60-90 min" : "Every 90-120 min";
  } else {
    spf="SPF 15-30";
    tip="Low burn risk, but high aging risk. Moisturize.";
    time="Every 120 min";
  }

  return { colorData:{r,g,b,ita:ita.toFixed(2)}, classification:{type,description:desc}, recommendations:{spfFactor:spf,reapplyFrequency:time,healthTip:tip}};
}

function rgbToLab(r,g,b){
  let r_n=r/255,g_n=g/255,b_n=b/255;
  r_n=(r_n>0.04045)?Math.pow((r_n+0.055)/1.055,2.4):r_n/12.92;
  g_n=(g_n>0.04045)?Math.pow((g_n+0.055)/1.055,2.4):g_n/12.92;
  b_n=(b_n>0.04045)?Math.pow((b_n+0.055)/1.055,2.4):b_n/12.92;
  let X=(r_n*0.4124+g_n*0.3576+b_n*0.1805)*100;
  let Y=(r_n*0.2126+g_n*0.7152+b_n*0.0722)*100;
  let Z=(r_n*0.0193+g_n*0.1192+b_n*0.9505)*100;
  const Rx=95.047,Ry=100,Rz=108.883;
  let x=X/Rx,y=Y/Ry,z=Z/Rz;
  x=(x>0.008856)?Math.pow(x,1/3):(7.787*x)+(16/116);
  y=(y>0.008856)?Math.pow(y,1/3):(7.787*y)+(16/116);
  z=(z>0.008856)?Math.pow(z,1/3):(7.787*z)+(16/116);
  return {L:(116*y)-16,a:500*(x-y),b:200*(y-z)};
}

// Websocket to measure the skin
let ws = null;

function initWebSocket() {
    ws = new WebSocket("ws://10.98.101.51:8765"); // Your ESP32 server

    ws.onopen = () => console.log("WebSocket connected");
    ws.onmessage = (event) => {
        const data = JSON.parse(event.data);
        handleIncomingData(data);
    };
    ws.onclose = () => {
        console.log("WebSocket disconnected. Reconnecting in 3s...");
        setTimeout(initWebSocket, 3000);
    };
}

function handleIncomingData(data) {
    if (data.type === "sensor") {
        console.log("Sensor data received:", data.readings);
        // Update your UI here (e.g., chart, skin analysis)
        displaySensorData(data.readings);
    } else if (data.type === "status") {
        console.log("Status:", data.message);
    }
}

function displaySensorData(readings) {
    // Example: just log first three channels
    const resultBox = document.getElementById("skin-analysis-result");
    const colorPreview = document.getElementById("color-preview");

    // For demonstration, map first 3 channels to RGB
    const r = readings[0] || 200;
    const g = readings[1] || 170;
    const b = readings[2] || 140;

    // Update UI
    colorPreview.style.backgroundColor = `rgb(${r},${g},${b})`;

    // Optionally run Fitzpatrick skin analysis logic
    const analysis = analyzeSkin(r, g, b, parseFloat(document.getElementById("uv-value").innerText) || 5);
    updateAnalysisUI(analysis);

    resultBox.style.display = "block";
}



const debugBtn = document.getElementById('debug-toggle-btn');
let debugEnabled = false;

debugBtn.addEventListener('click', () => {
  if (!debugEnabled) {
    // Enable debug mode
    enableDebugMode("ws://10.98.101.51:8765"); // Replace with your WS URL
    debugBtn.innerText = "Disable Debug Mode";
    debugEnabled = true;
  } else {
    // Disable debug mode
    disableDebugMode();
    debugBtn.innerText = "Enable Debug Mode";
    debugEnabled = false;
  }
});

document.getElementById("uv-test-button").addEventListener("click", () => {
    if (!ws || ws.readyState !== WebSocket.OPEN) {
        alert("WebSocket not connected yet!");
        return;
    }

    // Disable button during measurement
    const btn = document.getElementById("uv-test-button");
    btn.disabled = true;

    ws.send(JSON.stringify({ type: "command", action: "read_sensor" }));

    // Re-enable button after some delay if desired
    setTimeout(() => { btn.disabled = false; }, 5000);
});

window.addEventListener("load", () => {
    initWebSocket();
});

