"""
InduGuard Dashboard — Python Server (Fixed)
───────────────────────────────────────────
Flask + Flask-SocketIO + paho-mqtt
"""

import threading
import time
import json
import paho.mqtt.client as mqtt
from flask import Flask, send_from_directory
from flask_socketio import SocketIO

# ── CONFIG ────────────────────────────────────────────────────────────────────
BROKER_HOST = "98.130.28.156"
BROKER_PORT = 1883          # Raw TCP (server-side)
MQTT_USER   = "moambulance"
MQTT_PASS   = "P@$sw0rd2001"
# The user's screenshot shows 'induguard' while the .ino had 'indigurd'. 
# We'll subscribe to both to be safe.
MQTT_TOPICS = ["indigurd", "induguard"] 
HTTP_PORT   = 5000

# ── FLASK + SOCKET.IO ─────────────────────────────────────────────────────────
app = Flask(__name__, static_folder=".")
socketio = SocketIO(app, cors_allowed_origins="*", async_mode="threading")

@app.route("/")
def index():
    return send_from_directory(".", "index.html")

@app.route("/<path:filename>")
def static_files(filename):
    return send_from_directory(".", filename)

@socketio.on("connect")
def on_connect():
    print(f"🌐 Browser client connected")
    socketio.emit("status", {"connected": mqtt_client.is_connected()})

@socketio.on("disconnect")
def on_disconnect():
    print("❌ Browser client disconnected")

# ── MQTT CALLBACKS ────────────────────────────────────────────────────────────
def on_connect_cb(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print(f"✅ MQTT connected to {BROKER_HOST}")
        for topic in MQTT_TOPICS:
            client.subscribe(topic)
            print(f"📡 Subscribed to: {topic}")
        socketio.emit("status", {"connected": True})
    else:
        print(f"❌ MQTT connection failed (rc={rc})")

def on_message_cb(client, userdata, msg):
    try:
        payload = msg.payload.decode("utf-8")
        print(f"📩 [{msg.topic}]: {payload}")
        # Forward to browser
        socketio.emit("mqtt_message", {"topic": msg.topic, "payload": payload})
    except Exception as e:
        print(f"Error processing MQTT message: {e}")

def on_disconnect_cb(client, userdata, rc, properties=None):
    print(f"⚠ MQTT disconnected (rc={rc})")
    socketio.emit("status", {"connected": False})

# ── BUILD MQTT CLIENT ─────────────────────────────────────────────────────────
mqtt_client = mqtt.Client(
    mqtt.CallbackAPIVersion.VERSION2,
    client_id="induguard_py_server_fixed"
)
mqtt_client.username_pw_set(MQTT_USER, MQTT_PASS)
mqtt_client.on_connect    = on_connect_cb
mqtt_client.on_message    = on_message_cb
mqtt_client.on_disconnect = on_disconnect_cb

def start_mqtt():
    """Robust background loop."""
    print(f"🔄 Connecting to MQTT broker {BROKER_HOST}...")
    while True:
        try:
            if not mqtt_client.is_connected():
                mqtt_client.connect(BROKER_HOST, BROKER_PORT, keepalive=60)
                # Use loop_start() to handle networking in a background thread
                mqtt_client.loop_start()
            time.sleep(5)
        except Exception as e:
            print(f"MQTT connection error: {e}")
            time.sleep(5)

# ── MAIN ──────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    # Start MQTT in a proper thread
    mqtt_thread = threading.Thread(target=start_mqtt, daemon=True)
    mqtt_thread.start()

    print(f"\n╔══════════════════════════════════════╗")
    print(f"║   InduGuard Dashboard (Fixed)        ║")
    print(f"║   URL: http://localhost:{HTTP_PORT}        ║")
    print(f"║   Broker: {BROKER_HOST}      ║")
    print(f"╚══════════════════════════════════════╝\n")

    # Use monkey patching if necessary, but with threading it should be fine.
    # Flask-SocketIO handles its own event loop.
    socketio.run(app, host="0.0.0.0", port=HTTP_PORT, debug=False,
                 use_reloader=False, allow_unsafe_werkzeug=True)
