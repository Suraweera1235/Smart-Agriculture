// Step 8 - Rain sensor test (powered from D13)
const int RAIN_PIN   = 32;   // AO
const int RAIN_POWER = 13;   // VCC

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  pinMode(RAIN_POWER, OUTPUT);
  digitalWrite(RAIN_POWER, HIGH);   // power ON -> red LED should light
  Serial.println("Rain sensor test starting...");
}

void loop() {
  int raw = analogRead(RAIN_PIN);
  Serial.print("Rain raw: ");
  Serial.print(raw);
  if (raw > 3500)      Serial.println("   -> DRY");
  else if (raw > 2000) Serial.println("   -> A FEW DROPS");
  else                 Serial.println("   -> RAINING");
  delay(1000);
}