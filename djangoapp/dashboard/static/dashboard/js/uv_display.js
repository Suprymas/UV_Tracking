// constants from the HTML template
const CURRENT_ENDPOINT = UV_API_CURRENT_URL;
const HISTORY_ENDPOINT = UV_API_HISTORY_URL;
const SPF_ENDPOINT = typeof SPF_CALC_URL !== 'undefined' ? SPF_CALC_URL : '/dashboard/calculate_spf/';

const POLL_INTERVAL_MS = 60000;

const uvValueEl = document.getElementById('uv-value');
const uvCategoryEl = document.getElementById('uv-category');
const uvTimestampEl = document.getElementById('uv-timestamp');
const refreshBtn = document.getElementById('refresh-button');
const autoRefreshCheckbox = document.getElementById('auto-refresh');
const measureButton = document.getElementById('uv-test-button');
const countdownDisplay = document.getElementById('countdown-display');
const debugBtn = document.getElementById('debug-toggle-btn');
let debugEnabled = false;
let uvChart = null;
let ws = null;

// --- UV Index Categories ---
function uvCategory(uv) {
  if (uv == null || isNaN(uv)) return {label:'Unknown', level:'unknown'};
  if (uv <= 2) return {label:'Low', level:'low'};
  if (uv <= 7) return {label:'Mod/High', level:'moderate'};
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

// --- FIXED GRADIENT CHART LOGIC ---
async function fetchHistoryAndDraw() {
  const res = await fetch(HISTORY_ENDPOINT, {cache:'no-cache'});
  if (!res.ok) throw new Error('Network error');
  const json = await res.json();
  
  const labels = json.map(item => new Date(item.timestamp).toLocaleTimeString([], {hour: '2-digit', minute:'2-digit'}));
  const data = json.map(item => item.uv);
  
  // 1. Get the Canvas Context
  const ctx = document.getElementById('uv-chart').getContext('2d');
  
  // 2. DETECT ACTUAL HEIGHT (The Fix)
  // Instead of guessing 400px, we ask the canvas how tall it actually is.
  const chartHeight = ctx.canvas.height || 200; 

  // 3. Create Gradient mapped to the Y-Axis (0 to 14)
  // Canvas: 0 is Top (UV 14), chartHeight is Bottom (UV 0)
  let gradient = ctx.createLinearGradient(0, 0, 0, chartHeight);

  // Stop 0.0 (Top / UV 14): Extreme Red
  gradient.addColorStop(0, 'rgba(255, 0, 0, 0.8)'); 
  
  // Stop ~0.2 (UV 11): Start of Extreme
  gradient.addColorStop(0.2, 'rgba(255, 0, 0, 0.7)');

  // Stop ~0.4 (UV 8): Start of Very High (Orange)
  gradient.addColorStop(0.4, 'rgba(255, 140, 0, 0.7)');

  // Stop ~0.6 (UV 5): Moderate (Purple/Lilac)
  gradient.addColorStop(0.6, 'rgba(186, 85, 211, 0.7)');

  // Stop 1.0 (Bottom / UV 0): Low (Blue)
  gradient.addColorStop(1, 'rgba(0, 123, 255, 0.6)'); 

  if (uvChart) {
    uvChart.data.labels = labels;
    uvChart.data.datasets[0].data = data;
    // Apply the new dynamic gradient
    uvChart.data.datasets[0].backgroundColor = gradient;
    uvChart.update();
    return;
  }

  uvChart = new Chart(ctx, {
    type: 'line',
    data: { 
        labels, 
        datasets: [{ 
            label: 'UV Index', 
            data, 
            fill: true, 
            tension: 0.4, 
            pointRadius: 0, 
            borderWidth: 2,
            borderColor: '#888', // Grey line so the color pops
            backgroundColor: gradient 
        }] 
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: { 
          x:{ ticks:{ autoSkip: true, maxTicksLimit: 6 } }, 
          // IMPORTANT: Lock the Y-Axis to 14 so the gradient stays consistent!
          y:{ beginAtZero:true, min: 0, max: 14 } 
      },
      plugins: { legend:{ display:false } },
      interaction: { intersect: false, mode: 'index' },
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

// --- Skin Measurement Trigger ---
measureButton.addEventListener('click', startMeasurementSequence);

function startMeasurementSequence() {
  measureButton.disabled = true;
  const display = countdownDisplay;
  
  document.getElementById("skin-analysis-result").style.display = "none";
  
  display.innerText = "Scanning... 3";
  setTimeout(()=>display.innerText="Scanning... 2",1000);
  setTimeout(()=>display.innerText="Scanning... 1",2000);
  
  if(ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({ type: "command", action: "read_sensor" }));
  } else {
      display.innerText = "Error: Sensor disconnected";
      measureButton.disabled = false;
  }
  
  setTimeout(() => { 
      if(measureButton.disabled) {
          measureButton.disabled = false; 
          display.innerText = "";
      }
  }, 8000);
}

// --- WebSocket Setup ---
function initWebSocket() {
    if (typeof WEBSOCKET_URL === 'undefined') return;
    ws = new WebSocket(WEBSOCKET_URL);
    ws.onopen = () => console.log("WebSocket connected");
    ws.onmessage = (event) => handleIncomingData(JSON.parse(event.data));
    ws.onclose = () => setTimeout(initWebSocket, 3000);
}

function handleIncomingData(data) {
  if(data.type === "sensor" && debugEnabled) {
    const readings = data.readings.map((val, i) => `Ch${i+1}: ${val.toFixed(2)}`).join(", ");
    document.getElementById("debug-log").textContent = readings;
  }
  if (data.type === "sensor" && !debugEnabled) {
      sendReadingsToBackend(data.readings);
  }
}

function sendReadingsToBackend(readings) {
    const resultBox = document.getElementById("skin-analysis-result");
    const display = document.getElementById("countdown-display");
    const alertBox = document.getElementById("extreme-alert"); 
    const alertText = document.getElementById("extreme-alert-text");
    
    display.innerText = "Analyzing...";

    fetch(SPF_ENDPOINT, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ 'readings': readings })
    })
    .then(response => response.json())
    .then(data => {
        measureButton.disabled = false;
        display.innerText = "Scan Complete!";
        
        if(data.status === 'success') {
            document.getElementById("display-type").innerText = data.skin_type;
            document.getElementById("display-ita").innerText = data.ita_score + "°"; 
            document.getElementById("rec-spf").innerText = data.spf_recommendation;
            document.getElementById("rec-time").innerText = data.reapply_time; 
            document.getElementById("rec-tip").innerText = "Based on current UV: " + data.uv_index;

            // Warning Logic
            if (data.warning) {
                alertBox.style.display = "flex"; 
                if(alertText) alertText.innerText = data.warning;
                else alertBox.innerText = data.warning;
            } else {
                alertBox.style.display = "none";
            }

            // Color Preview
            const r = readings[0] ? Math.min(readings[0]*20, 255) : 200;
            const g = readings[1] ? Math.min(readings[1]*20, 255) : 170;
            const b = readings[2] ? Math.min(readings[2]*20, 255) : 140;
            document.getElementById("color-preview").style.backgroundColor = `rgb(${r},${g},${b})`;
            
            resultBox.style.display = "block";
        } else {
            display.innerText = "Error: " + (data.message || "Unknown");
        }
    })
    .catch(error => {
        console.error('Error:', error);
        measureButton.disabled = false;
        display.innerText = "Server Error";
    });
}

// --- Debug & Simulators ---
debugBtn.addEventListener('click', () => {
  if (!debugEnabled) {
    if(ws) ws.send(JSON.stringify({ type: "command", action: "debug_on" }));
    debugBtn.innerText = "Disable Debug Stream";
    debugEnabled = true;
  } else {
    if(ws) ws.send(JSON.stringify({ type: "command", action: "debug_off" }));
    debugBtn.innerText = "Enable Debug Stream";
    debugEnabled = false;
  }
});

window.simulateSensor = function(r,g,b) {
  const mockReadings = Array(18).fill(0);
  mockReadings[0] = r/20; mockReadings[1] = g/20; mockReadings[2] = b/20;
  for(let i=3; i<18; i++) mockReadings[i] = (r+g+b)/60; 
  sendReadingsToBackend(mockReadings);
}

window.addEventListener("load", initWebSocket);