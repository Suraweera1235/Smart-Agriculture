// Step 6 - Mini system: soil probe + LEDs + relay
// ---------- Pins ----------
const int SOIL_PIN   = 34;   // soil board AO
const int SOIL_POWER = 33;   // soil board VCC
const int RELAY      = 26;   // relay IN1
const int GREEN      = 25;
const int YELLOW     = 27;
const int RED        = 14;

// ---------- Calibration (update with your numbers) ----------
int DRY_VALUE = 4095;   // Raw in dry air
int WET_VALUE = 1500;   // Raw in water

// ---------- Watering rules ----------
const int START_BELOW   = 35;      // % - start watering
const int STOP_AT       = 60;      // % - stop watering
const unsigned long MAX_RUN_MS = 10000;  // 10 s safety limit (testing)
const unsigned long SOAK_MS    = 10000;  // wait 10 s after watering (testing)

// ---------- Relay type ----------
const bool RELAY_ACTIVE_LOW = true;   // most modules: LOW = ON

bool pumpOn = false;
unsigned long pumpStartedAt = 0;
unsigned long lastWateringEnded = 0;

// ---------- Helpers ----------
void setRelay(bool on) {
  if (on) {
    pinMode(RELAY, OUTPUT);
    digitalWrite(RELAY, RELAY_ACTIVE_LOW ? LOW : HIGH);
  } else if (RELAY_ACTIVE_LOW) {
    pinMode(RELAY, INPUT);            // release pin = reliably OFF
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
  Serial.print(">>> PUMP OFF (");
  Serial.print(reason);
  Serial.println(")");
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
  Serial.println("AquaSense mini system starting...");
}

void loop() {
  int raw = readSoilRaw();
  int moisture = constrain(map(raw, DRY_VALUE, WET_VALUE, 0, 100), 0, 100);
  bool sensorError = (raw < 50);   // 0 = wire problem

  Serial.print("Raw: "); Serial.print(raw);
  Serial.print("  Moisture: "); Serial.print(moisture); Serial.print("%");
  Serial.print("  Pump: "); Serial.println(pumpOn ? "ON" : "OFF");

  if (sensorError) {
    if (pumpOn) stopPump("sensor error");
    setLeds(false, false, true);                 // red
  }
  else if (pumpOn) {
    if (moisture >= STOP_AT)                        stopPump("soil wet");
    else if (millis() - pumpStartedAt > MAX_RUN_MS) stopPump("safety time limit");
    setLeds(false, pumpOn, false);               // yellow while pumping
  }
  else {
    bool soaking = (lastWateringEnded > 0) &&
                   (millis() - lastWateringEnded < SOAK_MS);
    if (moisture < START_BELOW && !soaking) {
      startPump();
      setLeds(false, true, false);               // yellow
    } else {
      setLeds(true, false, false);               // green
    }
  }

  delay(1000);
}