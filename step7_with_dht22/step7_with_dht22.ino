// Step 7 - Mini system + DHT22
#include <DHT.h>

// ---------- Pins ----------
const int SOIL_PIN   = 34;
const int SOIL_POWER = 33;
const int RELAY      = 26;
const int GREEN      = 25;
const int YELLOW     = 27;
const int RED        = 14;
const int DHT_PIN    = 4;

DHT dht(DHT_PIN, DHT22);

// ---------- Calibration (update with your numbers) ----------
int DRY_VALUE = 4095;
int WET_VALUE = 1500;

// ---------- Watering rules ----------
const int START_BELOW = 35;
const int STOP_AT     = 60;
const unsigned long MAX_RUN_MS = 10000;   // 10 s (testing)
const unsigned long SOAK_MS    = 10000;   // 10 s (testing)

const bool RELAY_ACTIVE_LOW = true;

bool pumpOn = false;
unsigned long pumpStartedAt = 0;
unsigned long lastWateringEnded = 0;
unsigned long lastDhtRead = 0;
float tempC = NAN, humidity = NAN;

// ---------- Helpers ----------
void setRelay(bool on) {
  if (on) {
    pinMode(RELAY, OUTPUT);
    digitalWrite(RELAY, RELAY_ACTIVE_LOW ? LOW : HIGH);
  } else if (RELAY_ACTIVE_LOW) {
    pinMode(RELAY, INPUT);
  } else {
    pinMode(RELAY, OUTPUT);
    digitalWrite(RELAY, LOW);
  }
}

void setLeds(bool g, bool y, bool r) {
  digitalWrite(GREEN, g);
  digitalWrite(YELLOW, y);
  digitalWrite(RED, r);
}

int readSoilRaw() {
  digitalWrite(SOIL_POWER, HIGH);
  delay(100);
  long total = 0;
  for (int i = 0; i < 10; i++) {
    total += analogRead(SOIL_PIN);
    delay(10);
  }
  digitalWrite(SOIL_POWER, LOW);
  return total / 10;
}

void readDht() {
  if (millis() - lastDhtRead < 2000) return;   // DHT22 max once per 2 s
  lastDhtRead = millis();
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (!isnan(h) && !isnan(t)) {
    humidity = h;
    tempC = t;
  }
}

void startPump() {
  setRelay(true);
  pumpOn = true;
  pumpStartedAt = millis();
  Serial.println(">>> PUMP ON");
}

void stopPump(const char* reason) {
  setRelay(false);
  pumpOn = false;
  lastWateringEnded = millis();
  Serial.print(">>> PUMP OFF ("); Serial.print(reason); Serial.println(")");
}

// ---------- Main ----------
void setup() {
  Serial.begin(115200);
  pinMode(SOIL_POWER, OUTPUT);
  digitalWrite(SOIL_POWER, LOW);
  pinMode(GREEN, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(RED, OUTPUT);
  analogReadResolution(12);
  setRelay(false);
  dht.begin();
  Serial.println("AquaSense (soil + DHT22) starting...");
}

void loop() {
  int raw = readSoilRaw();
  int moisture = constrain(map(raw, DRY_VALUE, WET_VALUE, 0, 100), 0, 100);
  bool sensorError = (raw < 50);
  readDht();

  Serial.print("Soil: "); Serial.print(moisture); Serial.print("% (raw ");
  Serial.print(raw); Serial.print(")  ");
  if (isnan(tempC)) {
    Serial.print("Temp: --  Hum: --  ");
  } else {
    Serial.print("Temp: "); Serial.print(tempC, 1); Serial.print("C  ");
    Serial.print("Hum: ");  Serial.print(humidity, 1); Serial.print("%  ");
  }
  Serial.print("Pump: "); Serial.println(pumpOn ? "ON" : "OFF");

  if (sensorError) {
    if (pumpOn) stopPump("sensor error");
    setLeds(false, false, true);
  }
  else if (pumpOn) {
    if (moisture >= STOP_AT)                        stopPump("soil wet");
    else if (millis() - pumpStartedAt > MAX_RUN_MS) stopPump("safety time limit");
    setLeds(false, pumpOn, false);
  }
  else {
    bool soaking = (lastWateringEnded > 0) &&
                   (millis() - lastWateringEnded < SOAK_MS);
    if (moisture < START_BELOW && !soaking) {
      startPump();
      setLeds(false, true, false);
    } else {
      setLeds(true, false, false);
    }
  }

  delay(1000);
}
