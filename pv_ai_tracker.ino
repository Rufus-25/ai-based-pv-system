// ---------- BLYNK ----------
#define BLYNK_TEMPLATE_ID "TMPL2DJ1fkmgv"
#define BLYNK_TEMPLATE_NAME "AI Based PV Tracker"
#define BLYNK_AUTH_TOKEN "nSy-ERVR0S3TiyRfCni6Ls1AawSie16s"

#include <Wire.h>
#include <Adafruit_INA219.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <time.h>
#include <ESP32Servo.h>

// ===== WiFi Credentials =====
char ssid[] = "Blessed";
char pass[] = "self.wifii";

// ===== Google Script ID =====
String GOOGLE_SCRIPT_ID = "AKfycbxY8J7kxI9-bSpXQEbt6NAqd3tDnahh0A-HPNP74MpnRMJ0xSv_DLVIhRjMa8XkvXFw-Q";

// ===== DHT Sensor =====
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ===== LDR Pins =====
#define LDR1_PIN 34
#define LDR2_PIN 35

// ===== Servo =====
Servo trackerServo;
#define SERVO_PIN 18

// ===== INA219 (Current/Voltage Sensor) =====
Adafruit_INA219 ina219;

// ===== Blynk Virtual Pins =====
#define VPIN_VOLTAGE     V0
#define VPIN_CURRENT     V1
#define VPIN_POWER       V2
#define VPIN_TEMPERATURE V3
#define VPIN_HUMIDITY    V4
#define VPIN_SERVO_ANGLE V5
#define VPIN_LDR1        V6
#define VPIN_LDR2        V7
#define VPIN_STATUS      V8

// ===== Time Setup =====
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;      // adjust to your timezone
const int daylightOffset_sec = 3600;  // adjust for DST if needed

// ===== Lookup Table =====
int lookupTable[12][24] = {
  {180,180,180,180,169,152,136,55,57,62,69,78,92,106,120,134,147,160,90,86,79,69,57,43},
  {180,180,180,180,169,152,136,56,58,62,69,79,92,106,120,133,147,159,90,86,79,69,57,43},
  {180,180,180,180,169,152,136,56,58,63,70,80,92,105,119,133,146,159,90,86,79,69,57,43},
  {180,180,180,180,169,152,58,57,59,64,71,80,92,104,118,132,145,158,169,86,79,69,57,43},
  {180,180,180,180,169,152,58,58,60,64,71,81,92,104,118,131,145,157,168,86,79,69,57,43},
  {180,180,180,180,169,152,58,58,60,65,72,81,92,104,117,131,145,157,168,86,79,69,57,43},
  {180,180,180,180,169,152,58,58,60,64,72,81,92,104,117,131,145,157,168,86,79,69,57,43},
  {180,180,180,180,169,152,58,57,59,64,71,81,92,104,118,132,145,158,168,86,79,69,57,43},
  {180,180,180,180,169,152,57,57,59,63,70,80,92,105,119,132,146,158,169,86,79,69,57,43},
  {180,180,180,180,169,152,136,56,58,63,70,79,92,106,119,133,147,159,90,86,79,69,57,43},
  {180,180,180,180,169,152,136,55,57,62,69,78,92,106,120,134,147,160,90,86,79,69,57,43},
  {180,180,180,180,169,152,136,55,57,62,69,78,92,107,120,134,148,160,90,86,79,69,57,43},
};

// ===== Predictive AI Function =====
int getPredictedAngle(int month, int hour) {
  if (hour < 6 || hour > 18) {
    return 90;  // Rest at night
  }
  return lookupTable[month - 1][hour];
}

// ===== Send to Google Sheets =====
void sendDataToGoogleSheets(String dateStr, int month, int hour, int angle,
                            float voltage, float current, float power,
                            float temp, float hum,
                            int ldr1, int ldr2) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec";
    url += "?date=" + dateStr;
    url += "&month=" + String(month);
    url += "&hour=" + String(hour);
    url += "&servo=" + String(angle);
    url += "&voltage=" + String(voltage, 3);
    url += "&current=" + String(current, 4);
    url += "&power=" + String(power, 4);
    url += "&temperature=" + String(temp, 2);
    url += "&humidity=" + String(hum, 2);
    url += "&ldr1=" + String(ldr1);
    url += "&ldr2=" + String(ldr2);

    http.begin(url.c_str());
    int code = http.GET();
    if (code > 0) {
      Serial.println("Data sent to Google Sheets. Code: " + String(code));
    } else {
      Serial.println("Failed to send data.");
    }
    http.end();
  }
}

void setup() {
  Serial.begin(115200);

  // Init DHT
  dht.begin();

  // Init INA219
  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }

  // Init Servo
  trackerServo.attach(SERVO_PIN);

  // ===== WiFi manual connection =====
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

  // ===== Init Blynk without WiFi takeover =====
  Blynk.config(BLYNK_AUTH_TOKEN);

  // ===== Init NTP =====
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {
  Blynk.run();

  // Get current time
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);
  int year = p_tm->tm_year + 1900;
  int month = p_tm->tm_mon + 1;
  int day = p_tm->tm_mday;
  int hour = p_tm->tm_hour;

  char dateStr[11];
  sprintf(dateStr, "%04d-%02d-%02d", year, month, day);

  // AI predicted angle
  int angle = getPredictedAngle(month, hour);
  trackerServo.write(angle);

  // Sensor readings
  float busVoltage = ina219.getBusVoltage_V();
  float current = ina219.getCurrent_mA() / 1000.0;
  float power = busVoltage * current;
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int ldr1 = analogRead(LDR1_PIN);
  int ldr2 = analogRead(LDR2_PIN);

  // Send to Google Sheets
  sendDataToGoogleSheets(dateStr, month, hour, angle,
                         busVoltage, current, power,
                         temperature, humidity, ldr1, ldr2);

  // Send to Blynk
  Blynk.virtualWrite(VPIN_VOLTAGE, busVoltage);
  Blynk.virtualWrite(VPIN_CURRENT, current);
  Blynk.virtualWrite(VPIN_POWER, power);
  Blynk.virtualWrite(VPIN_TEMPERATURE, temperature);
  Blynk.virtualWrite(VPIN_HUMIDITY, humidity);
  Blynk.virtualWrite(VPIN_SERVO_ANGLE, angle);
  Blynk.virtualWrite(VPIN_LDR1, ldr1);
  Blynk.virtualWrite(VPIN_LDR2, ldr2);
  Blynk.virtualWrite(VPIN_STATUS, "AI Tracking Active");

  delay(60000); // 1 min
}
