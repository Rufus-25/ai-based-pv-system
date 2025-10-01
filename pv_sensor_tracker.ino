// ---------- BLYNK ----------
#define BLYNK_TEMPLATE_ID "TMPL2DJ1fkmgv"
#define BLYNK_TEMPLATE_NAME "AI Based PV Tracker"
#define BLYNK_AUTH_TOKEN "nSy-ERVR0S3TiyRfCni6Ls1AawSie16s"

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
const long gmtOffset_sec = 3600;   // +1 hr for Nigeria
const int daylightOffset_sec = 0;

// ---------- Timers ----------
BlynkTimer timer;

// ---------- Blynk Virtual Pins ----------
#define VPIN_VOLTAGE V0
#define VPIN_CURRENT V1
#define VPIN_POWER V2
#define VPIN_TEMPERATURE V3
#define VPIN_HUMIDITY V4
#define VPIN_SERVO_ANGLE V5
#define VPIN_LDR1 V6
#define VPIN_LDR2 V7
#define VPIN_STATUS V8

// =================================================
// FUNCTIONS
// =================================================
String getDateHour() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "NA,NA";
  }
  char dateStr[11];
  char hourStr[6];
  strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
  strftime(hourStr, sizeof(hourStr), "%H:%M", &timeinfo);
  return String(dateStr) + "," + String(hourStr);
}

void uploadToGoogleSheets(String dateHour) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec?";
    url += "date=" + dateHour.substring(0, 10);
    url += "&hour=" + dateHour.substring(11);
    url += "&angle=" + String(currentAngle);
    url += "&voltage=" + String(panelVoltage, 2);
    url += "&temperature=" + String(temperature, 1);
    url += "&humidity=" + String(humidity, 1);
    url += "&current=" + String(panelCurrent, 3);
    url += "&power=" + String(panelPower, 2);

    http.begin(url.c_str());
    http.setTimeout(10000);
    int httpCode = http.GET();
    Serial.println("Google Sheets HTTP: " + String(httpCode));
    http.end();
  }
}

void measureSensors() {
  // Read INA219
  panelVoltage = ina219.getBusVoltage_V();
  panelCurrent = ina219.getCurrent_mA() / 1000.0;
  panelPower = panelVoltage * panelCurrent;
  
  // Read DHT
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  
  // Handle sensor errors
  if (isnan(temperature)) temperature = -999;
  if (isnan(humidity)) humidity = -999;
  
  // Read LDRs
  LDR1_value = analogRead(LDR1_PIN);
  LDR2_value = analogRead(LDR2_PIN);
}

void uploadToBlynk() {
  if (Blynk.connected()) {
    Blynk.virtualWrite(VPIN_SERVO_ANGLE, currentAngle);
    Blynk.virtualWrite(VPIN_VOLTAGE, panelVoltage);
    Blynk.virtualWrite(VPIN_CURRENT, panelCurrent);
    Blynk.virtualWrite(VPIN_POWER, panelPower);
    Blynk.virtualWrite(VPIN_TEMPERATURE, temperature);
    Blynk.virtualWrite(VPIN_HUMIDITY, humidity);
    Blynk.virtualWrite(VPIN_LDR1, LDR1_value);
    Blynk.virtualWrite(VPIN_LDR2, LDR2_value);
    Blynk.virtualWrite(VPIN_STATUS, 1); // System online
    
    Serial.println("Data sent to Blynk");
  } else {
    Serial.println("Blynk not connected - skipping update");
  }
}

void sendData() {
  Serial.println("=== Sending Data ===");
  measureSensors();
  
  // Print sensor readings
  Serial.printf("LDR1: %d, LDR2: %d, Angle: %d°\n", LDR1_value, LDR2_value, currentAngle);
  Serial.printf("Voltage: %.2fV, Current: %.3fA, Power: %.2fW\n", panelVoltage, panelCurrent, panelPower);
  Serial.printf("Temp: %.1f°C, Humidity: %.1f%%\n", temperature, humidity);
  
  String dateHour = getDateHour();
  uploadToGoogleSheets(dateHour);
  uploadToBlynk();
  Serial.println("=== Data Sent ===\n");
}

void checkBlynkConnection() {
  if (!Blynk.connected()) {
    Serial.println("Blynk disconnected - attempting to reconnect");
    Blynk.connect(5000);
  }
}

// Blynk connection events
BLYNK_CONNECTED() {
  Serial.println("✅ Blynk Connected Successfully!");
  // Sync all data when connected
  uploadToBlynk();
}

BLYNK_DISCONNECTED() {
  Serial.println("❌ Blynk Disconnected");
}

// =================================================
// SETUP
// =================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Solar Tracker Starting ===\n");

  // Initialize I2C
  Wire.begin(21, 22);
  
  // Initialize sensors
  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip!");
  } else {
    Serial.println("INA219 initialized");
  }
  
  dht.begin();
  Serial.println("DHT initialized");

  // Initialize servo
  panelServo.attach(SERVO_PIN);
  panelServo.write(currentAngle);
  Serial.println("Servo initialized");

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, pass);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n❌ WiFi Failed!");
  }

  // Initialize NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("NTP initialized");

  // Initialize Blynk
  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect(10000); // 10 second timeout
  
  // Setup timers
  timer.setInterval(5000L, sendData);           // Send data every 5 seconds
  timer.setInterval(60000L, checkBlynkConnection); // Check connection every minute

  Serial.println("\n=== System Ready ===\n");
}

// =================================================
// LOOP
// =================================================
void loop() {
  // --- Run Blynk and timers first ---
  Blynk.run();
  timer.run();

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
}
