# InduGuard — Industrial Safety Dashboard

InduGuard is a real-time industrial safety monitoring system designed for professional safety oversight. This dashboard visualizes live data from an ESP32-based hardware system equipped with Flame, Smoke, Vibration, Temperature, and Humidity sensors.

![InduGuard Dashboard Proof](C:\Users\ACER\.gemini\antigravity\brain\0ab2ae60-94de-40f0-bc76-0223b2070b15\dashboard_live_data_verification_1773556017050.png)

## 🚀 Quick Start

### 1. Prerequisites
- **Python 3.8+** installed on your system.
- Access to the internet (for loading modern UI assets from Google Fonts and Socket.IO CDN).

### 2. Installation
Clone the repository and install the required Python libraries:

```bash
git clone git@github.com:inventor-biswa/Induguard.git
cd Induguard/dashboard
pip install -r requirements.txt
```

### 3. Running the Dashboard
Start the Python bridge server:

```bash
python server.py
```

Now open your web browser and navigate to:
**[http://localhost:5000](http://localhost:5000)**

---

## 🛠️ System Architecture

1.  **Hardware (ESP32)**: Collects sensor data and publishes JSON packets to the MQTT broker.
2.  **MQTT Broker**: Acts as a central mailbox for data.
3.  **Python Bridge (`server.py`)**: Subscribes to the MQTT broker and forwards data to the browser using **Socket.IO**.
4.  **Web Frontend**: A premium, dark-mode dashboard providing:
    -   **Live Sensor Cards**: Instant visual alerts for Flame, Smoke, and Vibration.
    -   **Dynamic Charts**: Real-time history graphs for Temperature and Humidity.
    -   **Event Log**: A live scrollable audit log of all system events and data reception.

---

## 📂 Project Structure

- `dashboard/`
    - `server.py`: The Python Flask + Socket.IO server.
    - `index.html`: Dashboard structure with glassmorphism design.
    - `style.css`: Modern UI styling and animations.
    - `app.js`: Frontend logic for live data handling and chart rendering.
    - `requirements.txt`: Python package list.
- `final_working.ino`: The main firmware for the ESP32 hardware.

---

## 🎓 Academic Context
**Project**: Industrial Safety Monitoring System (InduGuard)
**Institution**: BJB Autonomous College
**Department**: Integrated M.Sc (IMSc)
**Student**: Biswa (Final Year Project)
