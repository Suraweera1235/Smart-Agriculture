// Step 11 - Pump test: 3 s ON, 5 s OFF
const int RELAY = 26;                 // relay IN1
const bool RELAY_ACTIVE_LOW = true;   // most relay modules: LOW = ON

void pumpOn() {
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, RELAY_ACTIVE_LOW ? LOW : HIGH);
}

void pumpOff() {
  if (RELAY_ACTIVE_LOW) {
    pinMode(RELAY, INPUT);            // release the pin = reliably OFF
  } else {
    pinMode(RELAY, OUTPUT);
    digitalWrite(RELAY, LOW);
  }
}

void setup() {
  Serial.begin(115200);
  pumpOff();                          // start with the pump OFF
  delay(500);
  Serial.println("Pump test starting...");
}

void loop() {
  Serial.println("PUMP ON");
  pumpOn();
  delay(3000);                        // run for 3 seconds

  Serial.println("PUMP OFF");
  pumpOff();
  delay(5000);                        // rest for 5 seconds
}