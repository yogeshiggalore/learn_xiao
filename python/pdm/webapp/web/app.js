let ws = null;

const WINDOW_SECONDS = 10;         // 10s window
const PLOT_REFRESH_MS = 1000;      // scroll every second
const MAX_PLOT_POINTS = 5000;      // downsample for Plotly performance

const el = (id) => document.getElementById(id);

let sampleRateHz = 16000;
let ring = [];                     // int16 samples (raw)
let totalSamples = 0;              // for time axis
let lastFrameAt = 0;

function nowStr() {
  const d = new Date();
  return d.toLocaleTimeString();
}

function log(msg, level="dim") {
  const box = el("logBox");
  const div = document.createElement("div");
  div.className = `logline ${level}`;
  div.textContent = `[${nowStr()}] ${msg}`;
  box.appendChild(div);
  box.scrollTop = box.scrollHeight;
}

async function api(path, opts={}) {
  const res = await fetch(path, opts);
  const txt = await res.text();
  let data = null;
  try { data = JSON.parse(txt); } catch { data = { raw: txt }; }
  if (!res.ok) throw new Error(data.error || `HTTP ${res.status}`);
  return data;
}

function setPills(connected, recording) {
  const conn = el("connPill");
  const rec = el("recPill");

  if (connected) {
    conn.textContent = "Connected";
    conn.classList.add("ok");
    conn.classList.remove("bad");
  } else {
    conn.textContent = "Disconnected";
    conn.classList.add("bad");
    conn.classList.remove("ok");
  }

  if (recording) {
    rec.textContent = "REC • ON";
    rec.classList.add("ok");
    rec.classList.remove("bad");
  } else {
    rec.textContent = "REC • OFF";
    rec.classList.add("bad");
    rec.classList.remove("ok");
  }
}

function setDot(hasData) {
  const dot = el("dot");
  const txt = el("miniText");
  if (hasData) {
    dot.style.background = "rgba(36,193,122,.9)";
    dot.style.boxShadow = "0 0 0 4px rgba(36,193,122,.12)";
    txt.textContent = "Streaming";
  } else {
    dot.style.background = "rgba(255,77,77,.85)";
    dot.style.boxShadow = "0 0 0 4px rgba(255,77,77,.12)";
    txt.textContent = "No data";
  }
}

async function loadPorts() {
  const ports = await api("/api/ports");
  const sel = el("portSelect");
  sel.innerHTML = "";

  if (!ports.length) {
    const opt = document.createElement("option");
    opt.value = "";
    opt.textContent = "No ports found";
    sel.appendChild(opt);
    el("portMeta").textContent = "—";
    return;
  }

  ports.forEach((p) => {
    const opt = document.createElement("option");
    opt.value = p.id;
    opt.textContent = p.label;
    opt.dataset.meta = JSON.stringify(p);
    sel.appendChild(opt);
  });

  sel.selectedIndex = 0;
  updatePortMeta();
}

function updatePortMeta() {
  const sel = el("portSelect");
  const opt = sel.options[sel.selectedIndex];
  if (!opt || !opt.dataset.meta) return;
  const p = JSON.parse(opt.dataset.meta);
  const manu = p.manufacturer || "-";
  const hwid = p.hwid || "-";
  el("portMeta").textContent = `Manufacturer: ${manu}  |  HWID: ${hwid}`;
}

function initPlot() {
  const N = Math.floor(sampleRateHz * WINDOW_SECONDS);
  const M = Math.min(N, MAX_PLOT_POINTS);

  // initialize in seconds so axis label is correct
  const x = Array.from({ length: M }, (_, i) => i / sampleRateHz);
  const y = Array(M).fill(0);

  const layout = {
    margin: { l: 55, r: 20, t: 18, b: 40 },
    paper_bgcolor: "white",
    plot_bgcolor: "white",

    xaxis: {
      title: "Time (s)",
      showgrid: true,
      gridcolor: "rgba(0,0,0,0.06)",
      zeroline: false,
      ticks: "outside",
      ticklen: 4,
    },

    yaxis: {
      title: "Amplitude (i16)",
      showgrid: true,
      gridcolor: "rgba(0,0,0,0.06)",
      zeroline: true,
      zerolinecolor: "rgba(0,0,0,0.18)",
      ticks: "outside",
      ticklen: 4,
      autorange: true,
    },

    showlegend: false,

    annotations: [{
      xref: "paper",
      yref: "paper",
      x: 0.01,
      y: 0.99,
      xanchor: "left",
      yanchor: "top",
      text: "min: —   max: —   avg: —",
      showarrow: false,
      font: { size: 12, color: "rgba(0,0,0,0.75)" },
      bgcolor: "rgba(255,255,255,0.85)",
      bordercolor: "rgba(0,0,0,0.08)",
      borderwidth: 1,
      borderpad: 6
    }],
  };

  const trace = {
    x,
    y,
    mode: "lines",
    line: { width: 2 },
    hoverinfo: "skip",
  };

  Plotly.newPlot("chart", [trace], layout, { displayModeBar: false, responsive: true });
}


function downsampleForPlot(samples) {
  if (samples.length <= MAX_PLOT_POINTS) return samples;

  const step = Math.ceil(samples.length / MAX_PLOT_POINTS);
  const out = [];
  for (let i = 0; i < samples.length; i += step) out.push(samples[i]);
  return out;
}

function updatePlotScroll() {
  // update plot once per second (scroll)
  const nWindow = Math.floor(sampleRateHz * WINDOW_SECONDS);
  const windowSamples = ring.length > nWindow ? ring.slice(ring.length - nWindow) : ring.slice();

  const y = downsampleForPlot(windowSamples);

  // time axis: last WINDOW_SECONDS mapped across y length
  const tEnd = totalSamples / sampleRateHz;
  const tStart = tEnd - WINDOW_SECONDS;
  const x = Array.from({length: y.length}, (_, i) => tStart + (i * WINDOW_SECONDS / Math.max(1, y.length-1)));

  Plotly.update("chart", {x: [x], y: [y]});
}

function openWS() {
  if (ws) ws.close();
  ws = new WebSocket(`ws://${location.host}/ws`);

  ws.onopen = () => log("WebSocket connected", "ok");
  ws.onclose = () => log("WebSocket disconnected", "bad");

  ws.onmessage = (evt) => {
    const msg = JSON.parse(evt.data);

    if (msg.type === "frame") {
      const data = msg.data;
      const samples = data.samples || [];

      if (data.sample_rate_hz && data.sample_rate_hz !== sampleRateHz) {
        sampleRateHz = data.sample_rate_hz;
        ring = [];
        totalSamples = 0;
        initPlot();
        log(`Sample rate updated to ${sampleRateHz} Hz`, "dim");
      }

      if (samples.length) {
        ring = ring.concat(samples);
        totalSamples += samples.length;

        // keep ring from growing forever
        const cap = Math.floor(sampleRateHz * WINDOW_SECONDS * 2); // keep some extra
        if (ring.length > cap) ring = ring.slice(ring.length - cap);

        lastFrameAt = Date.now();
      }

      if (data.timestamp_ms != null) {
        el("tsVal").textContent = `${data.timestamp_ms} ms`;
      }
    }

    if (msg.type === "log") {
      log(msg.data?.message || "log", msg.data?.level || "dim");
    }
  };
}

async function refreshStatus() {
  const st = await api("/api/status");

  setPills(st.connected, st.recording);
  el("connectBtn").disabled = st.connected;
  el("disconnectBtn").disabled = !st.connected;

  if (!st.connected) {
    el("tsVal").textContent = "—";
  }

  // update dot based on recent frames
  const hasData = (Date.now() - lastFrameAt) < 1500;
  setDot(st.connected && hasData);
}

async function connect() {
  const endpoint = el("portSelect").value;
  const baud = Number(el("baudInput").value);
  const sr = Number(el("srInput").value);

  if (!endpoint) {
    log("No port selected", "bad");
    return;
  }

  log(`Connecting to ${endpoint} @ ${baud}…`, "dim");

  await api("/api/connect", {
    method: "POST",
    headers: {"Content-Type": "application/json"},
    body: JSON.stringify({endpoint, baud, sample_rate_hz: sr})
  });

  // reset plotting buffers on connect
  sampleRateHz = sr;
  ring = [];
  totalSamples = 0;
  initPlot();

  log("Connected (recording started)", "ok");
  await refreshStatus();
}

async function disconnect() {
  log("Disconnecting…", "dim");
  await api("/api/disconnect", {method: "POST"});
  log("Disconnected (recording stopped)", "ok");
  await refreshStatus();
}

el("refreshBtn").addEventListener("click", loadPorts);
el("portSelect").addEventListener("change", updatePortMeta);
el("connectBtn").addEventListener("click", connect);
el("disconnectBtn").addEventListener("click", disconnect);

el("clearLogsBtn").addEventListener("click", () => {
  el("logBox").innerHTML = "";
  log("Logs cleared", "dim");
});

(async function boot() {
  log("UI loaded", "dim");
  await loadPorts();
  openWS();
  initPlot();
  await refreshStatus();

  // scroll plot once per second
  setInterval(updatePlotScroll, PLOT_REFRESH_MS);

  // keep status in sync
  setInterval(refreshStatus, 800);
})();
