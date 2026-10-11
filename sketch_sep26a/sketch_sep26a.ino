// Top rail power check - reads the rail voltage on D35
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
}

void loop() {
  int raw = analogRead(35);
  float volts = raw * 3.3 / 4095.0;
  Serial.print("Top rail: ");
  Serial.print(volts, 2);
  Serial.println(volts > 3.0 ? " V  -> POWER OK" : " V  -> NO POWER");
  delay(1000);
}