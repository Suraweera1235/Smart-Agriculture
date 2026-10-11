// Step 3 - Relay test
const int RELAY = 26;

void setup() {
  Serial.begin(115200);
  pinMode(RELAY, OUTPUT);
}

void loop() {
  digitalWrite(RELAY, LOW);
  Serial.println("Pin LOW");
  delay(2000);

  digitalWrite(RELAY, HIGH);
  Serial.println("Pin HIGH");
  delay(2000);
}
