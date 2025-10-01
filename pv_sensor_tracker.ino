// ---------- BLYNK ----------
#define BLYNK_TEMPLATE_ID   "TMPL2DJ1fkmgv"
#define BLYNK_TEMPLATE_NAME "AI Based PV Tracker"
#define BLYNK_AUTH_TOKEN    "nSy-ERVR0S3TiyRfCni6Ls1AawSie16s"

#include <WiFi.h>
#include <HTTPClient.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <DHT.h>
#include "time.h"

// ---------- WIFI ----------
char ssid[] = "Blessed";
char pass[] = "self.wifii";

// ---------- Google Sheets ----------
String GOOGLE_SCRIPT_ID = "AKfycbyTqApDZeDbCJouGWEE0G5KcotyoMO7vjyHDGXdDGJF9jWbKqw4Auzf4HG2E0mh9XYVQQ";

// ---------- Pins ----------
const int LDR1_PIN = 35;
const int LDR2_PIN = 34;
const int SERVO_PIN = 18;
#define DHTPIN 4
#define DHTTYPE DHT11

// ---------- Objects ----------
Servo panelServo;
Adafruit_INA219 ina219;
DHT dht(DHTPIN, DHTTYPE);

// ---------- Variables ----------
int LDR1_value = 0, LDR2_value = 0;
int currentAngle = 90;
int balancedAngle = 90;
const int tolerance = 20;
const int stepSize = 1;
const int minAngle = 0;
const int maxAngle = 180;

float panelVoltage = 0, panelCurrent = 0, panelPower = 0;
float temperature = 0, humidity = 0;

// ---------- NTP ----------
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;   // +1 hr for Nigeria
const int   daylightOffset_sec = 0;

// ---------- Timers ----------
BlynkTimer timer;

// =================================================
// FUNCTIONS
// =================================================
String getDateHour() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    return "NA,NA";
  }
  char dateStr[11];
  char hourStr[6];
  strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
  strftime(hourStr, sizeof(hourStr), "%H:%M", &timeinfo);
  return String(dateStr) + "," + String(hourStr);
}

void uploadToGoogleSheets(String dateHour) {
  if(WiFi.status()== WL_CONNECTED){
    HTTPClient http;
    String url = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec?";
    url += "date=" + dateHour.substring(0,10);
    url += "&hour=" + dateHour.substring(11);
    url += "&angle=" + String(currentAngle);
    url += "&voltage=" + String(panelVoltage,2);
    url += "&temperature=" + String(temperature,1);
    url += "&humidity=" + String(humidity,1);
    url += "&current=" + String(panelCurrent,3);
    url += "&power=" + String(panelPower,2);

    http.begin(url.c_str());
    int httpCode = http.GET();
    http.end();
  }
}

void uploadToBlynk() {
  Blynk.virtualWrite(V5, currentAngle);
  Blynk.virtualWrite(V0, panelVoltage);
  Blynk.virtualWrite(V1, panelCurrent);
  Blynk.virtualWrite(V2, panelPower);
  Blynk.virtualWrite(V3, temperature);
  Blynk.virtualWrite(V4, humidity);
}

void measureSensors() {
  panelVoltage = ina219.getBusVoltage_V();
  panelCurrent = ina219.getCurrent_mA() / 1000.0;
  panelPower   = ina219.getPower_mW() / 1000.0;
  temperature  = dht.readTemperature();
  humidity     = dht.readHumidity();
}

void sendData() {
  String dateHour = getDateHour();
  measureSensors();
  uploadToGoogleSheets(dateHour);
  uploadToBlynk();
}

// =================================================
// SETUP
// =================================================
void setup() {
  Serial.begin(115200);

  // Start WiFi first
  WiFi.begin(ssid, pass);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000UL) {
    delay(500);
  }

  // Now start Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Initialize sensors
  dht.begin();
  Wire.begin(21,22);
  ina219.begin();

  // Attach servo
  panelServo.attach(SERVO_PIN);
  panelServo.write(currentAngle);

  // Data upload every 10 sec
  timer.setInterval(10000L, sendData);
}

// =================================================
// LOOP
// =================================================
void loop() {
  // --- Continuous real-time tracking ---
  LDR1_value = analogRead(LDR1_PIN);
  LDR2_value = analogRead(LDR2_PIN);

  int diff = LDR1_value - LDR2_value;

  if (abs(diff) > tolerance) {
    if (diff > 0 && currentAngle < maxAngle) {
      currentAngle += stepSize;
    } else if (diff < 0 && currentAngle > minAngle) {
      currentAngle -= stepSize;
    }
    panelServo.write(currentAngle);
    delay(50);  // Smooth motion
  } else {
    balancedAngle = currentAngle; // store balanced angle
    delay(100); // brief pause when balanced
  }

  // --- Keep Blynk & timers running ---
  Blynk.run();
  timer.run();
}
