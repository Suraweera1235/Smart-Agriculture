// AquaSense SELF-TEST - checks every part and prints PASS / FAIL
#include <DHTesp.h>

const int GREEN = 25, YELLOW = 27, RED = 14;
const int RELAY = 26;
const int SOIL_PIN = 34, SOIL_POWER = 33;
const int DHT_PIN = 4;

DHTesp dht;

void line() { Serial.println("------------------------------------------"); }

void setup() {
  Serial.begin(115200);
  delay(1500);
  pinMode(GREEN, OUTPUT); pinMode(YELLOW, OUTPUT); pinMode(RED, OUTPUT);
  pinMode(SOIL_POWER, OUTPUT); digitalWrite(SOIL_POWER, LOW);
  pinMode(RELAY, INPUT);                       // relay released = OFF
  analogReadResolution(12);
  dht.setup(DHT_PIN, DHTesp::AUTO_DETECT);
}

void loop() {
  line(); Serial.println("AQUASENSE SELF-TEST"); line();

  // 1. LEDs
  Serial.println("1) LEDs: green -> yellow -> red (watch them)");
  int leds[] = {GREEN, YELLOW, RED};
  for (int i = 0; i < 3; i++) { digitalWrite(leds[i], HIGH); delay(700); digitalWrite(leds[i], LOW); }

  // 2. Relay
  Serial.println("2) RELAY: should CLICK ON now (IN1 LED on) ...");
  pinMode(RELAY, OUTPUT); digitalWrite(RELAY, LOW);
  delay(2000);
  Serial.println("   ... and CLICK OFF now");
  pinMode(RELAY, INPUT);
  delay(1500);

  // 3. Soil probe
  digitalWrite(SOIL_POWER, HIGH); delay(100);
  int raw = analogRead(SOIL_PIN);
  digitalWrite(SOIL_POWER, LOW);
  Serial.print("3) SOIL raw = "); Serial.print(raw);
  if (raw < 50)        Serial.println("  -> FAIL (0 = AO not connected to D34)");
  else if (raw > 4000) Serial.println("  -> OK if probe is in AIR (dip in water: should drop)");
  else                 Serial.println("  -> PASS (probe is reading moisture)");

  // 4. DHT22
  delay(2000);
  TempAndHumidity v = dht.getTempAndHumidity();
  Serial.print("4) DHT22 status = "); Serial.print(dht.getStatusString());
  if (dht.getStatus() == 0) {
    Serial.print("  -> PASS  Temp "); Serial.print(v.temperature, 1);
    Serial.print(" C, Hum "); Serial.print(v.humidity, 1); Serial.println(" %");
  } else if (String(dht.getStatusString()) == "TIMEOUT") {
    Serial.println("  -> FAIL: no answer (check + to 3V3, - to GND, out to D4)");
  } else {
    Serial.println("  -> FAIL: bad signal (loose wire)");
  }

  line(); Serial.println("Repeating in 5 seconds..."); Serial.println();
  delay(5000);
}