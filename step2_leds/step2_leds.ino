// Step 2 - Status LED test
const int GREEN  = 25;
const int YELLOW = 27;
const int RED    = 14;

void setup() {
  Serial.begin(115200);
  pinMode(GREEN, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(RED, OUTPUT);
}

void showOnly(int pin, const char* name) {
  digitalWrite(GREEN,  pin == GREEN);
  digitalWrite(YELLOW, pin == YELLOW);
  digitalWrite(RED,    pin == RED);
  Serial.println(name);
  delay(1000);
}

void loop() {
  showOnly(GREEN,  "GREEN  - soil OK");
  showOnly(YELLOW, "YELLOW - watering");
  showOnly(RED,    "RED    - fault");
}