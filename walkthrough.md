# InduGuard Dashboard — Walkthrough

## What Was Built

A live web dashboard for the **InduGuard** industrial safety monitor project (BJB Autonomous College, Final Year BSc).

### Architecture

```
ESP32 (final_working.ino)
    │  publishes JSON to MQTT topic "induguard"
    ▼
MQTT Broker (98.130.28.156:1883)
    │  paho-mqtt subscribes in Python (to "induguard" and "indigurd")
    ▼
server.py (Flask + Flask-SocketIO)
    │  forwards messages via Socket.IO
    ▼
Browser (localhost:5000)
    index.html + style.css + app.js
```

### Files Created

| File | Purpose |
|------|---------|
| [dashboard/server.py](file:///d:/Thynx/College%20Projects/induguard/dashboard/server.py) | Python server — serves dashboard + MQTT→SocketIO bridge (Fixed loop) |
| [dashboard/index.html](file:///d:/Thynx/College%20Projects/induguard/dashboard/index.html) | Dashboard layout (Uses Socket.IO CDN) |
| [dashboard/style.css](file:///d:/Thynx/College%20Projects/induguard/dashboard/style.css) | Premium dark-mode glassmorphism design |
| [dashboard/app.js](file:///d:/Thynx/College%20Projects/induguard/dashboard/app.js) | Live sensor updates (Socket.IO client) |
| [dashboard/requirements.txt](file:///d:/Thynx/College%20Projects/induguard/dashboard/requirements.txt) | Python dependencies |

### MQTT Data Handled

| Field | Description |
|-------|-------------|
| `flame` | "Flame Detected" / "Flame Not Detected" |
| `smoke` | "Smoke Detected" / "Smoke Not Detected" |
| `vibration` | "Vibration Detected" / "Vibration Not Detected" |
| `temperature` | Float (°C) |
| `humidity` | Float (%) |

## Verification

![Dashboard with live data](C:\Users\ACER\.gemini\antigravity\brain\0ab2ae60-94de-40f0-bc76-0223b2070b15\dashboard_live_data_verification_1773556017050.png)

- ✅ **MQTT OK** confirmed.
- ✅ **Live Data**: Receiving `temperature: 25.3` and `humidity: 50.9` live.
- ✅ **Topic Fix**: Successfully subscribing to `induguard` (the actual topic).
- ✅ **Charts**: Live trend lines populating.
- ✅ **Event Log**: Regular updates confirmed.

## How to Run

```bash
cd "d:\Thynx\College Projects\induguard\dashboard"
python server.py
# Open: http://localhost:5000
```
