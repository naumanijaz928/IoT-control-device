# ESP32 LED Control over MQTT 💡

A small but production-shaped IoT stack: control an LED connected to an **ESP32** from a
**React Native** app, via **FastAPI** and an **MQTT broker (Mosquitto)**.

No polling, no cloud, no database — the broker's *retained messages* hold the current
state, and *Last Will* gives free online/offline detection.

```
📱 React Native App ──REST + WebSocket──▶ 🐍 FastAPI ──MQTT──▶ 📬 Mosquitto ◀──MQTT──▶ 📟 ESP32 ──▶ 💡 LED
```

---

## ✨ Features

- **Instant control** — button press to LED change in milliseconds (MQTT pub/sub)
- **Truthful UI** — the app shows the state the ESP32 *confirmed*, not what was requested
- **Presence detection** — ESP32 goes offline → app greys out the device within ~15 s (MQTT Last Will)
- **Live updates** — WebSocket pushes state changes to every connected app
- **Resilient firmware** — non-blocking loop, Wi-Fi + MQTT auto-reconnect, reboot on Wi-Fi timeout
- **No database** — current state lives in the broker as retained messages

---

## 🧱 Tech Stack

| Layer | Tech |
|---|---|
| Device | ESP32 (Arduino core), `PubSubClient` |
| Broker | Eclipse Mosquitto 2.x (Docker) |
| Backend | Python 3.11+, FastAPI, `paho-mqtt`, WebSockets |
| Mobile | React Native (Expo) |

---

## 📡 MQTT Topics

| Topic | Publisher | Payload | Retained | Purpose |
|---|---|---|---|---|
| `home/<id>/cmd` | FastAPI (from app) | `on` / `off` | no | Command to the device |
| `home/<id>/state` | ESP32 | `on` / `off` | **yes** | Actual confirmed pin state |
| `home/<id>/status` | ESP32 / broker (LWT) | `online` / `offline` | **yes** | Device presence |

`<id>` is the `DEVICE_ID` set in the firmware (e.g. `led1`). FastAPI subscribes with
wildcards (`home/+/state`, `home/+/status`), so adding a device needs no backend logic changes.

---

## 🌐 HTTP API

| Method | Endpoint | Description |
|---|---|---|
| `GET` | `/devices` | List all devices with current `state` and `status` |
| `POST` | `/devices/{id}/cmd/{on\|off}` | Send a command (409 if device offline) |
| `WS` | `/ws` | Live updates; sends a full snapshot on connect |

Interactive docs: `http://<host>:8000/docs`

---

## 🔌 Hardware

- ESP32 dev board
- LED + **220Ω–330Ω resistor** (or use the onboard LED on GPIO 2 with no wiring)

```
GPIO 4 ──▶ 220Ω ──▶ LED (+ long leg)
GND    ◀────────── LED (− short leg)
```

---

## 🚀 Getting Started

All three machines (laptop, phone, ESP32) must be on the **same 2.4 GHz Wi-Fi** network.

### 1. Broker

```bash
# docker-compose (recommended)
docker compose up -d

# or plain docker (Windows cmd)
docker run -d --name mosquitto -p 1883:1883 ^
  -v "%cd%\mqtt\mosquitto.conf:/mosquitto/config/mosquitto.conf" ^
  -v mosquitto_data:/mosquitto/data eclipse-mosquitto
```

Verify:

```bash
docker exec -it mosquitto mosquitto_sub -t "home/#" -v        # terminal 1
docker exec -it mosquitto mosquitto_pub -t "home/led1/cmd" -m on   # terminal 2
```

### 2. Find your LAN IP & open the firewall

```bash
ipconfig        # Windows   → IPv4 Address under the Wi-Fi adapter
ip a            # Linux
ifconfig        # macOS
```

Windows (admin cmd):

```cmd
netsh advfirewall firewall add rule name="MQTT 1883" dir=in action=allow protocol=TCP localport=1883
netsh advfirewall firewall add rule name="FastAPI 8000" dir=in action=allow protocol=TCP localport=8000
```

Make sure the Wi-Fi network profile is set to **Private**.

### 3. Firmware (ESP32)

1. Arduino IDE → Library Manager → install **PubSubClient** (Nick O'Leary)
2. `cp firmware/led_mqtt/secrets.example.h firmware/led_mqtt/secrets.h` and fill in Wi-Fi + `MQTT_HOST`
3. Set `LED_PIN` (2 = onboard, 4 = external) in `led_mqtt.ino`
4. Flash, open Serial Monitor @ 115200 — expect `WiFi OK` → `MQTT connected`

### 4. Backend

```bash
cd backend
python -m venv .venv && .venv\Scripts\activate     # Windows
# source .venv/bin/activate                         # macOS/Linux
pip install -r requirements.txt
uvicorn main:app --host 0.0.0.0 --port 8000
```

Test at `http://localhost:8000/docs` → `POST /devices/led1/cmd/on`.

### 5. Mobile app

```bash
cd iot-app
npm install
# edit App.js → const HOST = "<your-laptop-ip>:8000";
npx expo start
```

Scan the QR code with **Expo Go**. Tap the button — the LED should change instantly.

---

## ➕ Adding another device

1. Flash a second ESP32 with `DEVICE_ID = "led2"`
2. Add `"led2"` to the `DEVICES` set in `backend/main.py`

That's it — topics, WebSocket events and the app list all follow automatically.

---

## 🔍 Troubleshooting

| Symptom | Likely cause |
|---|---|
| Serial stuck at `WiFi connecting....` | Wrong password, or network is 5 GHz only |
| `MQTT failed, rc=-2` / `rc=-4` | Can't reach broker → firewall, wrong IP, network set to Public |
| `MQTT failed, rc=5` | Broker refused → check `allow_anonymous true` |
| Serial says `LED -> on` but no light | LED reversed, or wrong `LED_PIN` |
| App: *Server unreachable* | Phone on mobile data, wrong `HOST`, or uvicorn not running |
| App: *led1 is offline* | ESP32 not connected to the broker |

---

## 🗺️ Roadmap

- [ ] Broker authentication (`mosquitto_passwd`, `allow_anonymous false`)
- [ ] API key / JWT on FastAPI
- [ ] Relay module → control real appliances
- [ ] **PZEM-004T** energy monitor → publish readings → SQLite/Postgres time-series
- [ ] Charts in the app
- [ ] Deploy broker + backend to a Raspberry Pi / VPS with TLS (port 8883)
- [ ] OTA firmware updates

---

## ⚠️ Safety note

The upcoming PZEM-004T stage connects to **220 V AC mains**. Always wire it de-energized,
use an enclosure, and double-check connections before applying power.

---

## 📄 License

MIT
