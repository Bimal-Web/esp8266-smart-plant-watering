#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// ---------- WIFI ----------
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

// ---------- PINS ----------
#define SOIL_PIN A0
#define RELAY_PIN D1

// Most relay modules are active LOW. If your pump behaves backwards, swap these.
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// ---------- SENSOR CALIBRATION ----------
// Measure these yourself: sensor in dry air = DRY_VALUE, sensor in water = WET_VALUE
int DRY_VALUE = 780;
int WET_VALUE = 380;

// ---------- WATERING SETTINGS ----------
int  moistureThreshold = 35;               // start watering below this %
int  moistureStop      = 55;               // stop watering above this %
const unsigned long MAX_PUMP_TIME_MS = 15000;     // safety: max 15 s per cycle
const unsigned long MIN_GAP_MS       = 600000UL;  // wait 10 min between cycles
const unsigned long READ_INTERVAL_MS = 2000;

// ---------- STATE ----------
bool autoMode = true;
bool pumpOn   = false;
int  moisturePercent = 0;
int  rawValue = 0;
unsigned long pumpStartTime = 0;
unsigned long lastPumpStop  = 0;
unsigned long lastRead      = 0;

ESP8266WebServer server(80);

// ---------- EEPROM ----------
void saveSettings() {
  EEPROM.write(0, 42);                 // magic number
  EEPROM.write(1, moistureThreshold);
  EEPROM.write(2, moistureStop);
  EEPROM.write(3, autoMode ? 1 : 0);
  EEPROM.commit();
}

void loadSettings() {
  if (EEPROM.read(0) == 42) {
    moistureThreshold = EEPROM.read(1);
    moistureStop      = EEPROM.read(2);
    autoMode          = EEPROM.read(3) == 1;
  }
}

// ---------- PUMP ----------
void setPump(bool on) {
  pumpOn = on;
  digitalWrite(RELAY_PIN, on ? RELAY_ON : RELAY_OFF);
  if (on) pumpStartTime = millis();
  else    lastPumpStop  = millis();
}

// ---------- SENSOR ----------
void readSoil() {
  long sum = 0;
  for (int i = 0; i < 10; i++) { sum += analogRead(SOIL_PIN); delay(5); }
  rawValue = sum / 10;
  moisturePercent = map(rawValue, DRY_VALUE, WET_VALUE, 0, 100);
  moisturePercent = constrain(moisturePercent, 0, 100);
}

// ---------- AUTO LOGIC ----------
void autoControl() {
  unsigned long now = millis();

  if (pumpOn) {
    // stop when wet enough OR safety timeout reached
    if (moisturePercent >= moistureStop || (now - pumpStartTime) >= MAX_PUMP_TIME_MS) {
      setPump(false);
    }
  } else {
    bool gapOk = (lastPumpStop == 0) || (now - lastPumpStop >= MIN_GAP_MS);
    if (moisturePercent < moistureThreshold && gapOk) {
      setPump(true);
    }
  }
}

// ---------- WEB PAGE ----------
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Smart Plant Watering</title>
<style>
body{font-family:Arial;background:#0f2a1d;color:#fff;text-align:center;margin:0;padding:20px}
.card{background:#1b4332;border-radius:16px;padding:20px;max-width:380px;margin:15px auto}
.big{font-size:56px;font-weight:bold;color:#95d5b2}
.bar{height:18px;background:#081c15;border-radius:9px;overflow:hidden}
.fill{height:100%;background:#52b788;width:0%;transition:.5s}
button{padding:12px 22px;margin:6px;border:0;border-radius:10px;font-size:16px;cursor:pointer}
.on{background:#40916c;color:#fff}.off{background:#d00000;color:#fff}.mode{background:#ffb703}
input{width:70px;padding:6px;border-radius:6px;border:0;text-align:center}
</style></head><body>
<h2>🌱 Smart Plant Watering</h2>
<div class="card">
  <div>Soil Moisture</div>
  <div class="big" id="m">--%</div>
  <div class="bar"><div class="fill" id="f"></div></div>
  <p>Pump: <b id="p">--</b> | Mode: <b id="mode">--</b></p>
</div>
<div class="card">
  <button class="mode" onclick="go('/mode')">Switch Auto / Manual</button><br>
  <button class="on"  onclick="go('/pump?s=1')">Pump ON</button>
  <button class="off" onclick="go('/pump?s=0')">Pump OFF</button>
</div>
<div class="card">
  <p>Start below <input id="t" type="number" min="0" max="100"> %</p>
  <p>Stop above <input id="s" type="number" min="0" max="100"> %</p>
  <button class="on" onclick="save()">Save</button>
</div>
<script>
function go(u){fetch(u).then(update)}
function save(){
  go('/set?t='+document.getElementById('t').value+'&s='+document.getElementById('s').value)
}
function update(){
  fetch('/status').then(r=>r.json()).then(d=>{
    document.getElementById('m').innerText=d.moisture+'%';
    document.getElementById('f').style.width=d.moisture+'%';
    document.getElementById('p').innerText=d.pump?'ON':'OFF';
    document.getElementById('mode').innerText=d.auto?'AUTO':'MANUAL';
    if(document.activeElement.tagName!='INPUT'){
      document.getElementById('t').value=d.threshold;
      document.getElementById('s').value=d.stop;
    }
  })
}
setInterval(update,2000);update();
</script></body></html>
)rawliteral";

void handleRoot() { server.send_P(200, "text/html", INDEX_HTML); }

void handleStatus() {
  String json = "{\"moisture\":" + String(moisturePercent) +
                ",\"raw\":" + String(rawValue) +
                ",\"pump\":" + String(pumpOn ? "true" : "false") +
                ",\"auto\":" + String(autoMode ? "true" : "false") +
                ",\"threshold\":" + String(moistureThreshold) +
                ",\"stop\":" + String(moistureStop) + "}";
  server.send(200, "application/json", json);
}

void handleMode() {
  autoMode = !autoMode;
  if (autoMode == false) setPump(false);
  saveSettings();
  server.send(200, "text/plain", "ok");
}

void handlePump() {
  if (!autoMode && server.hasArg("s")) {
    setPump(server.arg("s") == "1");
  }
  server.send(200, "text/plain", "ok");
}

void handleSet() {
  if (server.hasArg("t")) moistureThreshold = constrain(server.arg("t").toInt(), 0, 100);
  if (server.hasArg("s")) moistureStop      = constrain(server.arg("s").toInt(), 0, 100);
  if (moistureStop <= moistureThreshold) moistureStop = moistureThreshold + 10;
  saveSettings();
  server.send(200, "text/plain", "ok");
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);

  EEPROM.begin(16);
  loadSettings();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting");
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40) {
    delay(500); Serial.print("."); tries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected! Open: http://" + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi failed. Auto mode still works offline.");
  }

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/mode", handleMode);
  server.on("/pump", handlePump);
  server.on("/set", handleSet);
  server.begin();
}

// ---------- LOOP ----------
void loop() {
  server.handleClient();

  if (millis() - lastRead >= READ_INTERVAL_MS) {
    lastRead = millis();
    readSoil();
    Serial.printf("Raw: %d  Moisture: %d%%  Pump: %s\n",
                  rawValue, moisturePercent, pumpOn ? "ON" : "OFF");

    if (autoMode) autoControl();
    else if (pumpOn && (millis() - pumpStartTime) >= MAX_PUMP_TIME_MS) setPump(false); // safety in manual too
  }
}
