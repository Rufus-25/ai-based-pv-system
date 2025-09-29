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
String GOOGLE_SCRIPT_ID = "AKfycbyTqApDZeDbCJouGWEE0G5KcotyoMO7vjyHDGXdDGJF9jWbKqw4Auzf4HG2E0mh9XYVQQ"; // deployed Apps Script WebApp ID

// ---------- Pins ----------
const int LDR1_PIN = 34;
const int LDR2_PIN = 35;
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

// ---------- NTP for Date/Time ----------
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;

// ---------- Timers ----------
BlynkTimer timer;

// ========== FUNCTION: Get Date & Hour ==========
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

// ========== FUNCTION: Upload to Google Sheets ==========
void uploadToGoogleSheets(String dateHour) {
  if(WiFi.status()== WL_CONNECTED){
    HTTPClient http;
    String url = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec?";
    url += "date=" + dateHour.substring(0,10);
    url += "&hour=" + dateHour.substring(11);
    url += "&angle=" + String(balancedAngle);
    url += "&voltage=" + String(panelVoltage,2);
    url += "&temperature=" + String(temperature,1);
    url += "&humidity=" + String(humidity,1);
    url += "&current=" + String(panelCurrent,3);
    url += "&power=" + String(panelPower,2);

    http.begin(url.c_str());
    int httpCode = http.GET();
    if(httpCode > 0){
      Serial.println("Google Sheets Response: " + http.getString());
    }
    http.end();
  }
}

// ========== FUNCTION: Upload to Blynk ==========
void uploadToBlynk() {
  Blynk.virtualWrite(V5, balancedAngle);       // Servo angle
  Blynk.virtualWrite(V0, panelVoltage);        // Voltage
  Blynk.virtualWrite(V1, panelCurrent);        // Current
  Blynk.virtualWrite(V2, panelPower);          // Power
  Blynk.virtualWrite(V3, temperature);         // Temperature
  Blynk.virtualWrite(V4, humidity);            // Humidity
}

// ========== FUNCTION: Solar Tracking ==========
void trackSun() {
  LDR1_value = analogRead(LDR1_PIN);
  LDR2_value = analogRead(LDR2_PIN);
  int diff = LDR1_value - LDR2_value;

  if (abs(diff) <= tolerance) {
    balancedAngle = currentAngle;
  } else {
    if (diff > 0 && currentAngle < maxAngle) {
      currentAngle += stepSize;
    } else if (diff < 0 && currentAngle > minAngle) {
      currentAngle -= stepSize;
    }
    panelServo.write(currentAngle);
  }
}

// ========== FUNCTION: Measure Sensors ==========
void measureSensors() {
  panelVoltage = ina219.getBusVoltage_V();
  panelCurrent = ina219.getCurrent_mA() / 1000.0; // in Amps
  panelPower   = ina219.getPower_mW() / 1000.0;   // in Watts
  temperature  = dht.readTemperature();
  humidity     = dht.readHumidity();
}

// ========== TIMED TASK ==========
void sendData() {
  String dateHour = getDateHour();
  measureSensors();
  uploadToGoogleSheets(dateHour);
  uploadToBlynk();
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);

  // Servo
  panelServo.attach(SERVO_PIN);
  panelServo.write(currentAngle);

  // DHT
  dht.begin();

  // INA219
  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }

  // WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  // NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Timer: send every 10 seconds
  timer.setInterval(10000L, sendData);
}

// ========== LOOP ==========
void loop() {
  trackSun();
  Blynk.run();
  timer.run();
}
