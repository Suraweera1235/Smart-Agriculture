// Step 9 - DHT22 powered from GPIO19, data on GPIO18
#include <DHTesp.h>

const int DHT_POWER = 19;   // DHT22 +
const int DHT_PIN   = 18;   // DHT22 out
DHTesp dht;

void setup() {
  Serial.begin(115200);
  pinMode(DHT_POWER, OUTPUT);
  digitalWrite(DHT_POWER, HIGH);    // power the sensor
  delay(2000);                      // DHT22 needs ~2 s to wake up
  dht.setup(DHT_PIN, DHTesp::DHT22);
  Serial.println("DHT22 test starting...");
}

void loop() {
  delay(2500);
  TempAndHumidity v = dht.getTempAndHumidity();
  Serial.print("Status: ");
  Serial.print(dht.getStatusString());
  Serial.print("   Temp: ");
  Serial.print(v.temperature, 1);
  Serial.print(" C   Humidity: ");
  Serial.print(v.humidity, 1);
  Serial.println(" %");
}