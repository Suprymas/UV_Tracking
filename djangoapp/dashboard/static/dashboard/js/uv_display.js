// constants from the HTML template
const CURRENT_ENDPOINT = UV_API_CURRENT_URL;
const HISTORY_ENDPOINT = UV_API_HISTORY_URL;
// safe check: if we forgot to add the line in HTML, this won't crash
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

// Websocket to measure the skin
let ws = null;

// --- UV Index (Existing Logic) ---
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

// --- Skin Measurement Trigger ---
measureButton.addEventListener('click', startMeasurementSequence);

function startMeasurementSequence() {
  measureButton.disabled = true;
  const display = countdownDisplay;
  const resultBox = document.getElementById("skin-analysis-result");
  
  resultBox.style.display = "none";
  display.innerText = "Scanning... 3";
  
  setTimeout(()=>display.innerText="Scanning... 2",1000);
  setTimeout(()=>display.innerText="Scanning... 1",2000);
  
  // sending the command to the pi/esp32 to start reading
  if(ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({ type: "command", action: "read_sensor" }));
  } else {
      display.innerText = "Error: Sensor disconnected";
      measureButton.disabled = false;
  }
  
  // timeout reset if nothing happens
  setTimeout(() => { 
      if(measureButton.disabled) {
          measureButton.disabled = false; 
          display.innerText = "";
      }
  }, 8000);
}

// --- WebSocket Setup ---
function initWebSocket() {
    // relying on the WEBSOCKET_URL defined in the HTML file
    if (typeof WEBSOCKET_URL === 'undefined') {
        console.error("WEBSOCKET_URL not defined in HTML");
        return;
    }
    
    ws = new WebSocket(WEBSOCKET_URL);

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
  // handling debug mode text
  if(data.type === "sensor" && debugEnabled) {
    const readings = data.readings.map((val, i) => `Ch${i+1}: ${val.toFixed(6)}`).join("\n");
    document.getElementById("debug-log").textContent = `${readings}`;
  }

  // handling actual sensor data flow
  if (data.type === "sensor" && !debugEnabled) {
      console.log("Sensor data received, sending to backend...", data.readings);
      
      // NEW LOGIC: We do not calculate locally anymore. 
      // We send the data to Django to get the SPF recommendation.
      sendReadingsToBackend(data.readings);
      
  } else if (data.type === "status") {
      console.log("Status:", data.message);
  }
}

// --- Server Communication (Task 3.2 Logic) ---

function sendReadingsToBackend(readings) {
    const resultBox = document.getElementById("skin-analysis-result");
    const display = document.getElementById("countdown-display");
    const alertBox = document.getElementById("extreme-alert"); // Get the new box
    
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

            // --- EXTREME WARNING LOGIC ---
            if (data.warning) {
                // Show the red box
                alertBox.style.display = "block";
                alertBox.innerText = data.warning;
            } else {
                // Hide it if safe
                alertBox.style.display = "none";
            }
            // -----------------------------

            // Color preview logic
            const r = readings[0] ? Math.min(readings[0]*20, 255) : 200;
            const g = readings[1] ? Math.min(readings[1]*20, 255) : 170;
            const b = readings[2] ? Math.min(readings[2]*20, 255) : 140;
            document.getElementById("color-preview").style.backgroundColor = `rgb(${r},${g},${b})`;
            
            resultBox.style.display = "block";
        } else {
            display.innerText = "Error processing data";
        }
    })
    .catch(error => {
        console.error('Error:', error);
        measureButton.disabled = false;
        display.innerText = "Server Error";
    });
}


// --- Debug & Legacy Functions ---

debugBtn.addEventListener('click', () => {
  if (!debugEnabled) {
    measureButton.disabled = true;
    if(ws) ws.send(JSON.stringify({ type: "command", action: "debug_on" }));
    debugBtn.innerText = "Disable Debug Mode";
    debugEnabled = true;
  } else {
    if(ws) ws.send(JSON.stringify({ type: "command", action: "debug_off" }));
    debugBtn.innerText = "Enable Debug Mode";
    debugEnabled = false;
    measureButton.disabled = false;
    document.getElementById("debug-log").textContent = ``;
  }
});

// Keeping this for the manual "Simulate" buttons in the HTML
// (The buttons that say Type I, Type II, etc)
window.simulateSensor = function(r,g,b) {
  // we just mock the readings structure and send it to the backend function
  // so even the simulation tests the full server path
  const mockReadings = Array(18).fill(0);
  mockReadings[0] = r/20; // approximate reverse scaling
  mockReadings[1] = g/20;
  mockReadings[2] = b/20;
  // fill the rest with average
  for(let i=3; i<18; i++) mockReadings[i] = (r+g+b)/60; 
  
  sendReadingsToBackend(mockReadings);
}

window.addEventListener("load", () => {
    initWebSocket();
});