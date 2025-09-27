// RUFUS :)


#define BLYNK_TEMPLATE_ID "TMPL2DJ1fkmgv"
#define BLYNK_TEMPLATE_NAME "AI Based PV Tracker"
#define BLYNK_AUTH_TOKEN "nSy-ERVR0S3TiyRfCni6Ls1AawSie16s"

#include <Wire.h>
#include <Adafruit_INA219.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <BlynkSimpleEsp32.h>

// ====== CONFIG ======
char ssid[] = "Blessed";
char pass[] = "self.wifii";
char auth[] = "nSy-ERVR0S3TiyRfCni6Ls1AawSie16s";

String googleScriptURL = "https://script.google.com/macros/s/AKfycbxV-8SnDU1Mv37QbsWzf-7zmIrQF0oldpfpQLe1vAwhKXIPe2JVuuqFw73tTLiDz0UH/exec"; // e.g. https://script.google.com/macros/s/XXXX/exec

// ====== I2C pins for ESP32 (change if you use different pins) ======
#define I2C_SDA 21
#define I2C_SCL 22

// ====== INA219 & DHT ======
Adafruit_INA219 ina219;
#define DHTPIN 4       // DHT11 data pin (change if needed)
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ====== Setup ======
void setup() {
  Serial.begin(115200);
  delay(20);

  // Initialize I2C on chosen pins BEFORE starting INA219
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.printf("I2C started on SDA=%d SCL=%d\n", I2C_SDA, I2C_SCL);

  // Start WiFi (quick connect)
  WiFi.begin(ssid, pass);
  Serial.print("Connecting WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000UL) {
    Serial.print('.');
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) Serial.println("\nWiFi connected");
  else Serial.println("\nWiFi not connected (will try Blynk.connect later)");

  // Start Blynk (this will also try to connect to WiFi if not connected)
  Blynk.begin(auth, ssid, pass);

  // INA219
  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip - check wiring");
    while (1) { delay(10); }
  } else {
    Serial.println("INA219 found");
  }

  // DHT
  dht.begin();
  Serial.println("DHT initialized");
}

// ====== Loop ======
void loop() {
  Blynk.run();

  // Read INA219
  float busVoltage = ina219.getBusVoltage_V();     // V
  float current_mA = ina219.getCurrent_mA();       // mA
  float current_A = current_mA / 1000.0;           // A
  float power_mW = ina219.getPower_mW();           // mW
  float power_W = power_mW / 1000.0;               // W

  // Read DHT11
  float temp = dht.readTemperature();              // °C
  float hum  = dht.readHumidity();                 // %

  if (isnan(temp) || isnan(hum)) {
    Serial.println("DHT read failed");
    temp = 0;
    hum = 0;
  }

  // Send to Blynk (virtual pins)
  Blynk.virtualWrite(V0, busVoltage);
  Blynk.virtualWrite(V1, current_A);
  Blynk.virtualWrite(V2, power_W);
  Blynk.virtualWrite(V3, temp);
  Blynk.virtualWrite(V4, hum);

  // Log to Google Sheets (HTTP GET)
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = googleScriptURL +
      "?voltage=" + String(busVoltage, 3) +
      "&current=" + String(current_A, 4) +
      "&power="   + String(power_W, 4) +
      "&temp="    + String(temp, 2) +
      "&hum="     + String(hum, 2);
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.println("Sheets log HTTP code: " + String(httpCode));
    } else {
      Serial.println("Error sending to Google Sheets");
    }
    http.end();
  } else {
    Serial.println("WiFi offline - skip Sheets log");
  }

  // Debug
  Serial.printf("V=%.3f V  I=%.4f A  P=%.4f W  T=%.2f C  H=%.2f %%\n",
                busVoltage, current_A, power_W, temp, hum);

  delay(60000); // 1min interval
}

