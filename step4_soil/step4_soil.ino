// Step 4 - HW-080 soil moisture probe test
const int SOIL_PIN   = 34;   // AO from the soil control board
const int SOIL_POWER = 33;   // VCC of the soil control board

// Calibration - replace these with your real numbers later
int DRY_VALUE = 4095;   // probe in dry air
int WET_VALUE = 1500;   // probe in water

void setup() {
  Serial.begin(115200);
  pinMode(SOIL_POWER, OUTPUT);
  digitalWrite(SOIL_POWER, LOW);   // probe off
  analogReadResolution(12);        // readings 0 to 4095
  Serial.println("Soil probe test starting...");
}

int readSoilRaw() {
  digitalWrite(SOIL_POWER, HIGH);  // switch probe on
  delay(100);                      // let it settle
  long total = 0;
  for (int i = 0; i < 10; i++) {   // average 10 readings
    total += analogRead(SOIL_PIN);
    delay(10);
  }
  digitalWrite(SOIL_POWER, LOW);   // switch probe off (prevents corrosion)
  return total / 10;
}

void loop() {
  int raw = readSoilRaw();
  int percent = map(raw, DRY_VALUE, WET_VALUE, 0, 100);
  percent = constrain(percent, 0, 100);

  Serial.print("Raw: ");
  Serial.print(raw);
  Serial.print("   Moisture: ");
  Serial.print(percent);
  Serial.println("%");

  delay(2000);
}