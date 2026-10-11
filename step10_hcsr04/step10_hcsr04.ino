// Step 10 - HC-SR04 distance test
const int TRIG_PIN = 23;
const int ECHO_PIN = 22;

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  Serial.println("HC-SR04 test starting...");
}

float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);   // 10 µs pulse
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);        // wait max 30 ms
  if (duration == 0) return -1;                          // no echo
  return duration * 0.0343 / 2;                          // speed of sound
}

void loop() {
  float d = readDistanceCm();
  if (d < 0) Serial.println("No echo - check wiring");
  else {
    Serial.print("Distance: ");
    Serial.print(d, 1);
    Serial.println(" cm");
  }
  delay(500);
}