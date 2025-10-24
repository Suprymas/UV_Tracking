const CURRENT_ENDPOINT = UV_API_CURRENT_URL;
const HISTORY_ENDPOINT = UV_API_HISTORY_URL;
const POLL_INTERVAL_MS = 60000;

function uvCategory(uv) {
  if (uv === null || uv === undefined || Number.isNaN(uv)) return {label:'Unknown', level:'unknown'};
  if (uv <= 2) return {label:'Low', level:'low'};
  if (uv <= 5) return {label:'Moderate', level:'moderate'};
  if (uv <= 7) return {label:'High', level:'high'};
  if (uv <= 10) return {label:'Very High', level:'very-high'};
  return {label:'Extreme', level:'extreme'};
}

const uvValueEl = document.getElementById('uv-value');
const uvCategoryEl = document.getElementById('uv-category');
const uvTimestampEl = document.getElementById('uv-timestamp');
const refreshBtn = document.getElementById('refresh-button');
const autoRefreshCheckbox = document.getElementById('auto-refresh');
let uvChart = null;

function formatTimestamp(iso) {
  try { return new Date(iso).toLocaleString(); } catch { return iso; }
}

function updateCurrent(data) {
  const uv = (data && typeof data.uv === 'number') ? data.uv : null;
  const ts = (data && data.timestamp) ? data.timestamp : null;
  uvValueEl.textContent = uv === null ? '—' : uv.toFixed(1);
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
    data: {
      labels,
      datasets: [{ label: 'UV index', data, fill: true, tension: 0.3, pointRadius: 0, borderWidth: 2 }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        x: {

          ticks: {
            autoSkip: true,        // skips extra labels if too crowded

          }
        },
        y: {
          beginAtZero: true,
          suggestedMax: 12
        }
      },
      plugins: { legend: { display: false } },
      elements: { line: { borderJoinStyle: 'round' } }
    }
  });
}

let pollTimer = null;
function startPolling() {
  if (pollTimer) clearInterval(pollTimer);
  pollTimer = setInterval(() => { fetchCurrent(); fetchHistoryAndDraw(); }, POLL_INTERVAL_MS);
}
function stopPolling() { if (pollTimer) clearInterval(pollTimer); }

refreshBtn.addEventListener('click', () => { fetchCurrent(); fetchHistoryAndDraw(); });
autoRefreshCheckbox.addEventListener('change', () => {
  autoRefreshCheckbox.checked ? startPolling() : stopPolling();
});

(async function init() {
  await fetchCurrent();
  await fetchHistoryAndDraw();
  if (autoRefreshCheckbox.checked) startPolling();
})();
