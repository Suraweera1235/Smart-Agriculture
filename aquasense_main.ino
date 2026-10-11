/*
  AquaSense - Weather-Aware Smart Irrigation (Step 13: Wi-Fi + Firebase)
  Board: ESP32 Dev Module   |   Serial Monitor: 115200
  Library: DHTesp (by beegee_tokyo). WiFi / HTTPClient come with the ESP32 package.
*/
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <DHTesp.h>

// ===================== 👇 YOUR SETTINGS 👇 =====================
const char* WIFI_SSID = "AndroidAPA899";
const char* WIFI_PASS = "Senulids2222";
const char* DB_URL    = "https://green-f253f-default-rtdb.asia-southeast1.firebasedatabase.app";
// ===============================================================

// ---------------- MODES ----------------
const bool CAL_MODE = false;
const bool USE_TANK = true;

// ---------------- PINS ----------------
const int LED_GREEN = 25, LED_YELLOW = 27, LED_RED = 14;
const int RELAY     = 26;
const int SOIL_AO   = 34, SOIL_PWR = 33;
const int RAIN_AO   = 32, RAIN_PWR = 13;
const int DHT_DATA  = 18, DHT_PWR  = 19;
const int US_TRIG   = 23, US_ECHO  = 22;

// ---------------- CALIBRATION ----------------
int   SOIL_DRY_RAW   = 4095;
int   SOIL_WET_RAW   = 2000;
int   RAIN_WET_BELOW = 3500;
float TANK_EMPTY_CM  = 15.0;
float TANK_FULL_CM   = 4.0;
float PUMP_L_PER_S   = 0.025;

// ---------------- RULES ----------------
const int  START_BELOW  = 35;
const int  STOP_AT      = 60;
const int  TANK_MIN_PCT = 15;
const unsigned long MAX_RUN_MS  = 15000;
const unsigned long SOAK_MS     = 20000;
const unsigned long STARTUP_MS  = 10000;
const bool RELAY_ACTIVE_LOW = true;

// ---------------- CLOUD TIMING ----------------
const unsigned long LIVE_EVERY_MS    = 5000;
const unsigned long CONTROL_EVERY_MS = 3000;
const unsigned long HISTORY_EVERY_MS = 60000;
const unsigned long WIFI_RETRY_MS    = 30000;

DHTesp dht;
bool pumpRunning = false;
unsigned long pumpStartMs = 0, lastStopMs = 0, lastPrintMs = 0;
unsigned long lastLiveMs = 0, lastControlMs = 0, lastHistoryMs = 0, lastWifiTryMs = 0;
bool hasWateredBefore = false;
float totalLitres = 0;
String state = "START";
String pumpStartedBy = "auto";

bool manualMode = false;
bool manualPumpCmd = false;

int soilPct = 0, soilRaw = 0, rainRaw = 0, tankPct = 0;
bool raining = false, tankOk = false, dhtOk = false;
float dist = -1;
TempAndHumidity th = {NAN, NAN};

// ================= CLOUD HELPERS =================
bool wifiOk() { return WiFi.status() == WL_CONNECTED; }

int fbWrite(const char* method, const String& path, const String& json) {
  if (!wifiOk()) return -1;
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http; http.setTimeout(4000);
  if (!http.begin(client, String(DB_URL) + path + ".json")) return -1;
  http.addHeader("Content-Type", "application/json");
  int code = http.sendRequest(method, json);
  http.end();
  return code;
}

String fbRead(const String& path) {
  if (!wifiOk()) return "";
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http; http.setTimeout(4000);
  if (!http.begin(client, String(DB_URL) + path + ".json")) return "";
  int code = http.GET();
  String body = (code == 200) ? http.getString() : "";
  http.end();
  return body;
}

String num(float v, int decimals) { return isnan(v) ? "null" : String(v, decimals); }

String readingsJson() {
  String j = "{";
  j += "\"soil\":" + String(soilPct) + ",\"soilRaw\":" + String(soilRaw);
  j += ",\"rain\":" + String(raining ? "true" : "false") + ",\"rainRaw\":" + String(rainRaw);
  j += ",\"tank\":" + (tankOk ? String(tankPct) : String("null"));
  j += ",\"tankCm\":" + (tankOk ? String(dist, 1) : String("null"));
  j += ",\"temp\":" + (dhtOk ? num(th.temperature, 1) : String("null"));
  j += ",\"hum\":" + (dhtOk ? num(th.humidity, 0) : String("null"));
  j += ",\"pump\":" + String(pumpRunning ? "true" : "false");
  j += ",\"ts\":{\".sv\":\"timestamp\"}";
  return j;
}

void sendLive() {
  if (!wifiOk()) return;                                   // no Wi-Fi: skip quietly
  String j = readingsJson();
  j += ",\"state\":\"" + state + "\"";
  j += ",\"mode\":\"" + String(manualMode ? "manual" : "auto") + "\"";
  j += ",\"litresTotal\":" + String(totalLitres, 3);
  j += ",\"rssi\":" + String(WiFi.RSSI()) + "}";
  int code = fbWrite("PUT", "/aquasense/live", j);
  if (code != 200) Serial.printf("[cloud] live upload failed (%d)\n", code);
}

void sendHistory() {
  String j = readingsJson() + "}";
  fbWrite("POST", "/aquasense/history", j);
}

void sendEvent(float secs, float litres, const char* reason) {
  String j = "{\"secs\":" + String(secs, 1) + ",\"litres\":" + String(litres, 3);
  j += ",\"reason\":\"" + String(reason) + "\",\"by\":\"" + pumpStartedBy + "\"";
  j += ",\"ts\":{\".sv\":\"timestamp\"}}";
  fbWrite("POST", "/aquasense/events", j);
}

void readControl() {
  String body = fbRead("/aquasense/control");
  if (body.length() == 0 || body == "null") return;
  manualMode    = body.indexOf("\"manual\"") >= 0;
  manualPumpCmd = body.indexOf("\"pump\":true") >= 0;
}

void setManualPumpOffInCloud() { fbWrite("PATCH", "/aquasense/control", "{\"pump\":false}"); }

// ================= HARDWARE HELPERS =================
void setRelay(bool on) {
  if (on) { pinMode(RELAY, OUTPUT); digitalWrite(RELAY, RELAY_ACTIVE_LOW ? LOW : HIGH); }
  else if (RELAY_ACTIVE_LOW) pinMode(RELAY, INPUT);
  else { pinMode(RELAY, OUTPUT); digitalWrite(RELAY, LOW); }
}

void leds(bool g, bool y, bool r) {
  digitalWrite(LED_GREEN, g); digitalWrite(LED_YELLOW, y); digitalWrite(LED_RED, r);
}

int readAnalogPowered(int aoPin, int pwrPin) {
  digitalWrite(pwrPin, HIGH); delay(150);
  analogRead(aoPin);
  long sum = 0;
  for (int i = 0; i < 10; i++) { sum += analogRead(aoPin); delay(2); }
  digitalWrite(pwrPin, LOW);
  return sum / 10;
}

float readDistanceCm() {
  digitalWrite(US_TRIG, LOW); delayMicroseconds(2);
  digitalWrite(US_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(US_TRIG, LOW);
  unsigned long t = pulseIn(US_ECHO, HIGH, 30000);
  if (t == 0) return -1;
  return t * 0.0343 / 2.0;
}

int toPercent(float v, float zeroAt, float hundredAt) {
  float p = (v - zeroAt) * 100.0 / (hundredAt - zeroAt);
  return constrain((int)p, 0, 100);
}

void startPump(const char* by) {
  setRelay(true); pumpRunning = true; pumpStartMs = millis(); pumpStartedBy = by;
  Serial.printf(">>> PUMP ON (%s)\n", by);
  sendLive();
}

void stopPump(const char* reason) {
  setRelay(false); pumpRunning = false;
  lastStopMs = millis(); hasWateredBefore = true;
  float secs = (lastStopMs - pumpStartMs) / 1000.0;
  float litres = secs * PUMP_L_PER_S;
  totalLitres += litres;
  Serial.printf(">>> PUMP OFF (%s) | this run %.3f L | total %.3f L\n", reason, litres, totalLitres);
  sendEvent(secs, litres, reason);
  sendLive();
}

// ================= WI-FI (with diagnostics) =================
void onWifiFail(WiFiEvent_t e, WiFiEventInfo_t info) {
  int r = info.wifi_sta_disconnected.reason;
  Serial.printf("[wifi] failed, reason %d = %s\n", r,
    r == 201 ? "HOTSPOT NOT FOUND (5 GHz / off / wrong name)" :
    (r == 15 || r == 202 || r == 2 || r == 204) ? "WRONG PASSWORD or WPA3 security" :
    "weak signal / other");
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.disconnect(true);
  delay(300);

  // 1) Can the ESP32 see the hotspot at all?
  Serial.println("[wifi] scanning...");
  int n = WiFi.scanNetworks();
  bool seen = false;
  for (int i = 0; i < n; i++) {
    int enc = WiFi.encryptionType(i);
    Serial.printf("   '%s'  %d dBm%s\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i),
                  (enc == 6 || enc == 7) ? "  <-- WPA3, change hotspot to WPA2" : "");
    if (WiFi.SSID(i) == WIFI_SSID) seen = true;
  }
  WiFi.scanDelete();
  Serial.printf("[wifi] '%s' %s\n", WIFI_SSID,
                seen ? "IS visible" : "NOT visible -> hotspot is 5 GHz, off, or name differs");

  // 2) Try to connect (up to 15 s)
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[wifi] connecting");
  for (int i = 0; i < 30 && !wifiOk(); i++) { delay(500); Serial.print("."); }
  if (wifiOk()) Serial.printf("\n[wifi] CONNECTED  IP %s\n", WiFi.localIP().toString().c_str());
  else          Serial.println("\n[wifi] not connected - system keeps working offline, will retry every 30 s");
}

// ================= SETUP =================
void setup() {
  setRelay(false);
  Serial.begin(115200);
  pinMode(LED_GREEN, OUTPUT); pinMode(LED_YELLOW, OUTPUT); pinMode(LED_RED, OUTPUT);
  pinMode(SOIL_PWR, OUTPUT); pinMode(RAIN_PWR, OUTPUT);
  digitalWrite(SOIL_PWR, LOW); digitalWrite(RAIN_PWR, LOW);
  pinMode(DHT_PWR, OUTPUT);  digitalWrite(DHT_PWR, HIGH);
  pinMode(US_TRIG, OUTPUT);  pinMode(US_ECHO, INPUT);
  analogSetAttenuation(ADC_11db);
  dht.setup(DHT_DATA, DHTesp::DHT22);
  leds(1, 1, 1); delay(2000); leds(0, 0, 0);
  WiFi.onEvent(onWifiFail, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  connectWifi();
  lastWifiTryMs = millis();
  Serial.println("AquaSense IoT started");
}

// ================= LOOP =================
void loop() {
  unsigned long now = millis();

  // Wi-Fi retry (clean, every 30 s)
  if (!wifiOk() && now - lastWifiTryMs > WIFI_RETRY_MS) {
    lastWifiTryMs = now;
    Serial.println("[wifi] retrying...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }

  soilRaw = readAnalogPowered(SOIL_AO, SOIL_PWR);
  rainRaw = readAnalogPowered(RAIN_AO, RAIN_PWR);

  static float lastDist = -1; static int echoMiss = 0;
  float d = readDistanceCm();
  if (d > 0) { lastDist = d; echoMiss = 0; } else if (++echoMiss >= 3) lastDist = -1;
  dist = lastDist;

  static int dhtFails = 0; static unsigned long lastDht = 0;
  if (now - lastDht > 2500) {
    lastDht = now;
    TempAndHumidity r = dht.getTempAndHumidity();
    if (dht.getStatus() == DHTesp::ERROR_NONE) { th = r; dhtOk = true; dhtFails = 0; }
    else if (++dhtFails >= 3) dhtOk = false;
  }

  soilPct = toPercent(soilRaw, SOIL_DRY_RAW, SOIL_WET_RAW);
  raining = rainRaw < RAIN_WET_BELOW;
  tankOk  = dist > 0;
  tankPct = tankOk ? toPercent(dist, TANK_EMPTY_CM, TANK_FULL_CM) : 0;
  bool tankLow  = USE_TANK && (!tankOk || tankPct < TANK_MIN_PCT);
  bool soilErr  = soilRaw < 200;
  bool soaking  = hasWateredBefore && (now - lastStopMs < SOAK_MS);
  bool starting = now < STARTUP_MS;
  bool blink    = (now / 500) % 2;

  if (now - lastControlMs > CONTROL_EVERY_MS) { lastControlMs = now; readControl(); }

  if (CAL_MODE) {
    if (pumpRunning) stopPump("cal mode");
    state = "CALIBRATION"; leds(blink, blink, blink);
  }
  else if (soilErr) {
    if (pumpRunning) stopPump("soil sensor error");
    state = "SOIL SENSOR ERROR"; leds(0, 0, blink);
  }
  else if (manualMode) {
    if (pumpRunning) {
      if (!manualPumpCmd)                                stopPump("manual off");
      else if (millis() - pumpStartMs > MAX_RUN_MS)    { stopPump("SAFETY max run time"); manualPumpCmd = false; setManualPumpOffInCloud(); }
      else if (tankLow)                                { stopPump("tank low");            manualPumpCmd = false; setManualPumpOffInCloud(); }
    } else if (manualPumpCmd) {
      if (tankLow) { manualPumpCmd = false; setManualPumpOffInCloud(); }
      else startPump("manual");
    }
    if (pumpRunning)  { state = "MANUAL - WATERING"; leds(0, 1, 0); }
    else if (tankLow) { state = "MANUAL - TANK LOW"; leds(0, 0, 1); }
    else              { state = "MANUAL - IDLE";     leds(1, 0, 0); }
  }
  else if (pumpRunning) {
    if      (soilPct >= STOP_AT)                  stopPump("soil wet enough");
    else if (millis() - pumpStartMs > MAX_RUN_MS) stopPump("SAFETY max run time");
    else if (tankLow)                             stopPump("tank low");
    else if (raining)                             stopPump("rain started");
    state = pumpRunning ? "WATERING" : "STOPPED"; leds(0, 1, 0);
  }
  else if (starting)                           { state = "STARTING UP - waiting"; leds(blink, 0, 0); }
  else if (tankLow)                            { state = tankOk ? "TANK LOW - pump blocked" : "TANK SENSOR NO ECHO - pump blocked"; leds(0, 0, 1); }
  else if (soilPct < START_BELOW && raining)   { state = "DRY but RAINING - skip"; leds(0, blink, 0); }
  else if (soilPct < START_BELOW && !soaking)  { startPump("auto"); state = "WATERING"; leds(0, 1, 0); }
  else                                         { state = soaking ? "SOAKING" : "OK"; leds(1, 0, 0); } 

  if (now - lastLiveMs > LIVE_EVERY_MS)       { lastLiveMs = now; sendLive(); }
  if (now - lastHistoryMs > HISTORY_EVERY_MS) { lastHistoryMs = now; sendHistory(); }

  if (now - lastPrintMs > 2000) {
    lastPrintMs = now;
    Serial.printf("Soil %3d%% (raw %4d) | Rain %s (raw %4d) | Tank ", soilPct, soilRaw, raining ? "YES" : "no ", rainRaw);
    if (tankOk) Serial.printf("%3d%% (%.1f cm)", tankPct, dist); else Serial.print(" -- (no echo)");
    if (dhtOk)  Serial.printf(" | %.1f C %.0f%%", th.temperature, th.humidity); else Serial.print(" | DHT error");
    Serial.printf(" | Pump %s | %s | %s | WiFi %s\n", pumpRunning ? "ON " : "off",
                  manualMode ? "MANUAL" : "AUTO", state.c_str(), wifiOk() ? "ok" : "--");
  }
  delay(100);
}