// Relay-only test: tries 3 ways of driving the relay
const int RELAY = 26;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Relay test starting...");
}

void loop() {
  // Mode 1: pin LOW
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);
  Serial.println("1) Pin LOW       -> listen for click");
  delay(3000);

  // Mode 2: pin HIGH
  digitalWrite(RELAY, HIGH);
  Serial.println("2) Pin HIGH      -> listen for click");
  delay(3000);

  // Mode 3: pin released (floating)
  pinMode(RELAY, INPUT);
  Serial.println("3) Pin released  -> listen for click");
  delay(3000);
}
