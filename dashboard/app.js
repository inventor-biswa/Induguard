// ================================================================
//   InduGuard Dashboard — app.js
//   Connects to Python Flask-SocketIO server (server.py)
//   which bridges MQTT → Socket.IO for the browser
// ================================================================

const MAX_POINTS = 30;

// ---------- STATE ----------
let tempHistory = [];
let humHistory  = [];
let labels      = [];
let tempChart, humChart;

// ================================================================
//   CLOCK
// ================================================================
function updateClock() {
  document.getElementById("clock").textContent =
    new Date().toLocaleTimeString("en-IN", { hour12: false });
}
setInterval(updateClock, 1000);
updateClock();

// ================================================================
//   CHART SETUP
// ================================================================
function initCharts() {
  const base = {
    type: "line",
    options: {
      responsive: true,
      maintainAspectRatio: false,
      animation: { duration: 400 },
      plugins: { legend: { display: false } },
      scales: {
        x: {
          ticks: { color: "#475569", font: { size: 9 }, maxTicksLimit: 6 },
          grid:  { color: "rgba(255,255,255,0.04)" }
        },
        y: {
          ticks: { color: "#475569", font: { size: 9 } },
          grid:  { color: "rgba(255,255,255,0.04)" }
        }
      }
    }
  };

  tempChart = new Chart(document.getElementById("tempChart"), {
    ...base,
    data: {
      labels,
      datasets: [{
        data: tempHistory,
        borderColor: "#f59e0b",
        backgroundColor: "rgba(245,158,11,0.08)",
        borderWidth: 2, fill: true, tension: 0.4,
        pointRadius: 2, pointHoverRadius: 5
      }]
    }
  });

  humChart = new Chart(document.getElementById("humChart"), {
    ...base,
    data: {
      labels,
      datasets: [{
        data: humHistory,
        borderColor: "#3b82f6",
        backgroundColor: "rgba(59,130,246,0.08)",
        borderWidth: 2, fill: true, tension: 0.4,
        pointRadius: 2, pointHoverRadius: 5
      }]
    }
  });
}

// ================================================================
//   UPDATE SENSOR CARDS
// ================================================================
function updateCard(key, rawValue) {
  const val   = document.getElementById(`val-${key}`);
  const badge = document.getElementById(`badge-${key}`);
  const card  = document.getElementById(`card-${key}`);
  const ring  = document.getElementById(`ring-${key}`);
  if (!val) return false;

  const binary = ["flame", "smoke", "vibration"];
  if (binary.includes(key)) {
    const isAlert = rawValue.toLowerCase().includes("detected") &&
                    !rawValue.toLowerCase().includes("not");
    const isWarm  = rawValue.toLowerCase().includes("warming");

    val.textContent  = isWarm ? "Warming Up…" : (isAlert ? "⚠ DETECTED" : "Clear");
    val.style.color  = isAlert ? "#ef4444" : isWarm ? "#f59e0b" : "#22c55e";
    badge.textContent = isWarm ? "WARM UP" : isAlert ? "ALERT" : "SAFE";
    badge.className   = "card-badge " + (isAlert ? "alert" : isWarm ? "warn" : "ok");
    card.className    = "card " + (isAlert ? "danger" : "safe");
    ring.className    = "card-ring " + (isAlert ? "alert" : isWarm ? "warn" : "ok");

    if (isAlert) {
      card.style.animation = "none";
      setTimeout(() => card.style.animation = "shake 0.4s", 10);
    }
    return isAlert;
  }

  // Numeric
  const n = parseFloat(rawValue);
  val.textContent   = isNaN(n) ? "Error" : n.toFixed(1);
  badge.textContent = key === "temp" ? "°C" : "%RH";
  badge.className   = "card-badge ok";
  card.className    = "card safe";
  ring.className    = "card-ring ok";
  return false;
}

// ================================================================
//   PROCESS INCOMING MQTT PAYLOAD
// ================================================================
function processMessage(payload) {
  let data;
  try { data = JSON.parse(payload); }
  catch (e) { addLog("Parse error: " + payload, "warn"); return; }

  const now = new Date().toLocaleTimeString("en-IN", { hour12: false });
  let anyAlert = false;

  if (data.flame     !== undefined) anyAlert |= updateCard("flame",     data.flame);
  if (data.smoke     !== undefined) anyAlert |= updateCard("smoke",     data.smoke);
  if (data.vibration !== undefined) anyAlert |= updateCard("vibration", data.vibration);

  if (data.temperature !== undefined) {
    updateCard("temp", data.temperature.toString());
    tempHistory.push(parseFloat(data.temperature) || 0);
    if (tempHistory.length > MAX_POINTS) tempHistory.shift();
  }
  if (data.humidity !== undefined) {
    updateCard("humidity", data.humidity.toString());
    humHistory.push(parseFloat(data.humidity) || 0);
    if (humHistory.length > MAX_POINTS) humHistory.shift();
    labels.push(now);
    if (labels.length > MAX_POINTS) labels.shift();
  }

  if (tempChart) tempChart.update();
  if (humChart)  humChart.update();

  // Alert banner
  const banner = document.getElementById("alertBanner");
  if (anyAlert) {
    let msg = "";
    if (data.flame?.includes("Flame Detected"))          msg += " 🔥 FLAME";
    if (data.smoke?.includes("Smoke Detected"))           msg += " 💨 SMOKE";
    if (data.vibration?.includes("Vibration Detected"))  msg += " 📳 VIBRATION";
    document.getElementById("alertText").textContent = "ALERT:" + msg;
    banner.classList.add("active");
    addLog(`⚠ ALERT –${msg}`, "danger");
  } else {
    banner.classList.remove("active");
    addLog(`✓ Data received at ${now}`, "ok");
  }
}

// ================================================================
//   EVENT LOG
// ================================================================
function addLog(msg, type = "info") {
  const scroll = document.getElementById("logScroll");
  const entry  = document.createElement("div");
  const now    = new Date().toLocaleTimeString("en-IN", { hour12: false });
  entry.className   = `log-entry ${type}`;
  entry.textContent = `[${now}] ${msg}`;
  scroll.appendChild(entry);
  scroll.scrollTop = scroll.scrollHeight;
  while (scroll.children.length > 80) scroll.removeChild(scroll.firstChild);
}

function clearLog() {
  document.getElementById("logScroll").innerHTML = "";
}

// ================================================================
//   CONNECTION STATUS
// ================================================================
function setStatus(connected, label) {
  const pill = document.getElementById("mqttStatus");
  const txt  = document.getElementById("mqttStatusText");
  pill.className  = "status-pill " + (connected ? "connected" : "disconnected");
  txt.textContent = connected ? (label || "Connected") : "Disconnected";
}

// ================================================================
//   SOCKET.IO CONNECTION  (to Python server.py)
// ================================================================
function connectSocketIO() {
  addLog("Connecting to Python server…", "info");

  // Socket.IO client is loaded from the Python server automatically
  const socket = io();   // connects back to same host/port

  socket.on("connect", () => {
    setStatus(true, "SocketIO");
    addLog("✅ Connected to Python server!", "ok");
  });

  socket.on("disconnect", () => {
    setStatus(false);
    addLog("Disconnected from server — reconnecting…", "warn");
  });

  socket.on("status", (data) => {
    setStatus(data.connected, data.connected ? "MQTT OK" : "MQTT Disconnected");
  });

  socket.on("mqtt_message", (data) => {
    processMessage(data.payload);
  });

  socket.on("connect_error", (err) => {
    addLog("Connection error: " + err.message, "warn");
  });
}

// ================================================================
//   SHAKE ANIMATION
// ================================================================
const _style = document.createElement("style");
_style.textContent = `
@keyframes shake {
  0%,100%{transform:translateX(0)}
  20%{transform:translateX(-6px)} 40%{transform:translateX(6px)}
  60%{transform:translateX(-4px)} 80%{transform:translateX(4px)}
}`;
document.head.appendChild(_style);

// ================================================================
//   LOAD CHART.JS THEN INIT
// ================================================================
const _chartScript = document.createElement("script");
_chartScript.src = "https://cdn.jsdelivr.net/npm/chart.js@4.4.2/dist/chart.umd.min.js";
_chartScript.onload  = () => { initCharts(); connectSocketIO(); };
_chartScript.onerror = () => { addLog("Chart.js failed to load", "warn"); connectSocketIO(); };
document.head.appendChild(_chartScript);
