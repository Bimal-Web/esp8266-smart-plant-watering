# esp8266-smart-plant-watering
Automatic plant watering system using ESP8266, a soil moisture sensor and a relay, with a built-in web dashboard for auto/manual control over Wi-Fi.


# 🌱 ESP8266 Smart Plant Watering


An IoT automatic plant watering system built on the **ESP8266**. It reads soil moisture with a capacitive sensor and switches a water pump through a relay. A **web dashboard runs on the ESP8266 itself**, so you don't need a cloud account, an app, or a subscription. Open the board's IP address in any browser on your Wi-Fi to monitor and control your plant.

---

## 📑 Table of Contents

- [Features](#-features)
- [Hardware Required](#-hardware-required)
- [Wiring](#-wiring)
- [Software Setup](#-software-setup)
- [Sensor Calibration](#-sensor-calibration)
- [Using the Dashboard](#-using-the-dashboard)
- [Configuration](#-configuration)
- [HTTP API](#-http-api)
- [How It Works](#-how-it-works)
- [Safety Features](#-safety-features)
- [Troubleshooting](#-troubleshooting)
- [Project Structure](#-project-structure)
- [Roadmap](#-roadmap)
- [Contributing](#-contributing)
- [License](#-license)

---

## ✨ Features

- 📊 Live soil moisture reading in percent
- 🤖 **Auto mode** with adjustable start and stop thresholds
- 🎛️ **Manual mode** with pump ON/OFF buttons
- 📱 Mobile-friendly web dashboard served directly from the ESP8266
- 💾 Settings saved in EEPROM, so they survive power loss
- 🛡️ Pump safety timeout and a minimum gap between watering cycles
- 📡 Auto mode keeps working even if Wi-Fi goes down
- 🔌 Simple JSON endpoint for integration with other tools

---

## 🧰 Hardware Required

| Component | Notes |
|---|---|
| ESP8266 board | NodeMCU v3 or Wemos D1 Mini |
| Capacitive soil moisture sensor | Preferred over resistive sensors, which corrode quickly |
| 1-channel relay module (5V) | Active-LOW modules are the most common |
| DC water pump (5V or 12V) | Small submersible pump with a silicone tube |
| Separate pump power supply | Must match the pump's voltage |
| 1N4007 diode | Flyback protection across the pump |
| Jumper wires, breadboard | |
| Water container and tubing | |

---

## 🔌 Wiring

| Part | ESP8266 Pin |
|---|---|
| Soil sensor AOUT | `A0` |
| Soil sensor VCC | `3.3V` |
| Soil sensor GND | `GND` |
| Relay IN | `D1` (GPIO5) |
| Relay VCC | `VIN` (5V) |
| Relay GND | `GND` |

### Pump circuit

The pump is powered from **its own supply**, never from the ESP8266 pins. The relay only acts as a switch in the pump's power line.

```
Pump supply (+) ───► Relay COM
                     Relay NO ───► Pump (+)
Pump (−) ───────────► Pump supply (−)

1N4007 diode across the pump terminals:
stripe (cathode) to Pump (+), other end to Pump (−)
```

> ⚠️ **Safety:** Keep the electronics away from water. Use only low-voltage supplies for the pump. Do not switch mains voltage unless you know how to do it safely.

> 💡 Connect the pump supply's ground to the ESP8266 ground only if your relay module needs it. Most opto-isolated relay modules don't.

---

## 💻 Software Setup

### Option A: Arduino IDE

1. Open **File → Preferences** and add this URL to *Additional Boards Manager URLs*:
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
2. Open **Tools → Board → Boards Manager**, search for **esp8266** and install it.
3. Select your board: **NodeMCU 1.0 (ESP-12E Module)** or **LOLIN(WEMOS) D1 mini**.
4. Create a sketch folder and put the code in an `.ino` file (the Arduino IDE opens a sketch from its `.ino` file).
5. Edit the Wi-Fi credentials in the code:
   ```cpp
   const char* WIFI_SSID = "YOUR_WIFI_NAME";
   const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
   ```
6. Click **Upload**.
7. Open the **Serial Monitor** at `115200` baud and note the IP address it prints.

### Option B: PlatformIO

1. Clone the repository:
   ```bash
   git clone https://github.com/Bimal-Web/esp8266-smart-plant-watering.git
   cd esp8266-smart-plant-watering
   ```
2. Put `plant_watering.cpp` in `src/` and rename it to `main.cpp`.
3. Create `platformio.ini`:
   ```ini
   [env:nodemcuv2]
   platform = espressif8266
   board = nodemcuv2
   framework = arduino
   monitor_speed = 115200
   ```
4. Edit your Wi-Fi credentials in `main.cpp`.
5. Build and upload:
   ```bash
   pio run --target upload
   pio device monitor
   ```

---

## 🎯 Sensor Calibration

Every sensor reads slightly differently, so calibrate yours for accurate percentages.

1. Upload the code and open the Serial Monitor.
2. Hold the sensor in **dry air** and note the `Raw` value. Set it as `DRY_VALUE`.
3. Put the sensor in a glass of **water**, only up to the line marked on the sensor. Note the `Raw` value and set it as `WET_VALUE`.
4. Re-upload the code.

```cpp
int DRY_VALUE = 780;   // sensor in dry air
int WET_VALUE = 380;   // sensor in water
```

---

## 🖥️ Using the Dashboard

1. Connect your phone or laptop to the **same Wi-Fi network** as the ESP8266.
2. Open the IP address from the Serial Monitor, for example `http://192.168.1.50`.
3. The dashboard shows:
   - Current soil moisture (percent and bar)
   - Pump state (ON/OFF)
   - Current mode (AUTO/MANUAL)
4. Use the controls:
   - **Switch Auto / Manual** toggles the mode
   - **Pump ON / OFF** works in manual mode only
   - **Start below / Stop above** sets the watering thresholds, then press **Save**

<!-- Add a screenshot after you take one:
![Dashboard](images/dashboard.png)
-->

---

## ⚙️ Configuration

All settings are at the top of the code.

| Setting | Default | Description |
|---|---|---|
| `moistureThreshold` | `35` | Start watering below this percent |
| `moistureStop` | `55` | Stop watering above this percent |
| `MAX_PUMP_TIME_MS` | `15000` | Maximum pump run time per cycle (15 s) |
| `MIN_GAP_MS` | `600000` | Minimum wait between cycles (10 min) |
| `READ_INTERVAL_MS` | `2000` | Sensor read interval (2 s) |
| `RELAY_ON` / `RELAY_OFF` | `LOW` / `HIGH` | Swap them if your relay is active-HIGH |

The thresholds and the mode can also be changed from the dashboard, and they are stored in EEPROM.

---

## 🌐 HTTP API

The ESP8266 exposes these endpoints, which are handy for scripts or home automation.

| Endpoint | Description |
|---|---|
| `GET /` | Web dashboard |
| `GET /status` | JSON with moisture, pump state, mode and thresholds |
| `GET /mode` | Toggle Auto / Manual |
| `GET /pump?s=1` or `s=0` | Pump ON/OFF (manual mode only) |
| `GET /set?t=35&s=55` | Set start (`t`) and stop (`s`) thresholds |

Example response from `/status`:

```json
{
  "moisture": 42,
  "raw": 610,
  "pump": false,
  "auto": true,
  "threshold": 35,
  "stop": 55
}
```

---

## 🧠 How It Works

```
Read sensor (average of 10 samples) ──► Convert to % moisture
                  │
        Auto mode ON?
         │            │
        Yes           No
         │            └─► Wait for manual pump commands
         ▼
Moisture < start threshold AND gap since last cycle ≥ 10 min?
         │
        Yes ──► Pump ON
                  │
Moisture ≥ stop threshold OR 15 s elapsed? ──► Pump OFF
```

---

## 🛡️ Safety Features

- **Pump timeout:** the pump never runs longer than 15 seconds per cycle, so a failed sensor can't flood the plant. This also applies in manual mode.
- **Minimum gap:** at least 10 minutes between automatic cycles, so water can soak into the soil before the next reading.
- **Hysteresis:** separate start and stop thresholds stop the pump from rapidly switching on and off.
- **Safe boot state:** the relay is turned OFF at startup.
- **Offline operation:** if Wi-Fi fails, auto mode keeps running.

> 🔒 The dashboard has **no password**. Use it only on a trusted home network and do not expose the ESP8266 to the internet through port forwarding.

---

## 🛠️ Troubleshooting

| Problem | Likely cause and fix |
|---|---|
| Pump runs when it should be off | Your relay is active-HIGH. Swap `RELAY_ON` and `RELAY_OFF`. |
| Moisture always 0% or 100% | Sensor not calibrated. Redo the calibration steps. |
| Readings jump around | Use a capacitive sensor, keep wires short, and add a 100 nF capacitor between sensor VCC and GND. |
| Can't open the dashboard | Check that the phone and ESP8266 are on the same Wi-Fi. Check the IP in the Serial Monitor. |
| ESP8266 resets when the pump starts | The pump draws too much power. Use a separate supply and add the flyback diode. |
| Wi-Fi won't connect | The ESP8266 supports only 2.4 GHz networks. Check the SSID and password. |
| Relay clicks but the pump doesn't run | Check the COM/NO wiring and the pump supply voltage. |

---

## 📁 Project Structure

```
esp8266-smart-plant-watering/
├── README.md
├── LICENSE
├── plant_watering.cpp     # main firmware (rename to src/main.cpp for PlatformIO)
├── platformio.ini         # optional, for PlatformIO users
└── images/
    ├── dashboard.png
    └── wiring-diagram.png
```

---

## 🗺️ Roadmap

- [ ] Blynk or MQTT support for remote control from anywhere
- [ ] Water tank level sensor to protect the pump from running dry
- [ ] OLED display for local readout
- [ ] DHT11/DHT22 temperature and humidity logging
- [ ] Password-protected dashboard
- [ ] Deep sleep mode for battery or solar use
- [ ] Watering history chart

---

## 🤝 Contributing

Contributions are welcome.

1. Fork the repository
2. Create a branch: `git checkout -b feature/my-feature`
3. Commit your changes: `git commit -m "Add my feature"`
4. Push the branch: `git push origin feature/my-feature`
5. Open a Pull Request

---

## 📄 License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.

---

## 👤 Author

**Bimal** ([@Bimal-Web](https://github.com/Bimal-Web))

If this project helped you, give it a ⭐ on GitHub.
