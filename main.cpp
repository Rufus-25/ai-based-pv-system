// Full integration with google sheet and blynk dashboard

#define BLYNK_TEMPLATE_ID "TMPL2DJ1fkmgv"
#define BLYNK_TEMPLATE_NAME "AI Based PV Tracker"
#define BLYNK_AUTH_TOKEN "nSy-ERVR0S3TiyRfCni6Ls1AawSie16s"

#include <Wire.h>
#include <Adafruit_INA219.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <BlynkSimpleEsp32.h>
#include <ArduinoJson.h>

// ====== CONFIG ======
char ssid[] = "Blessed";
char pass[] = "self.wifii";
char auth[] = BLYNK_AUTH_TOKEN;

String googleScriptURL = "https://script.google.com/macros/s/AKfycbxV-8SnDU1Mv37QbsWzf-7zmIrQF0oldpfpQLe1vAwhKXIPe2JVuuqFw73tTLiDz0UH/exec";

// ====== Hardware Pins ======
#define I2C_SDA 21
#define I2C_SCL 22
#define DHTPIN 4
#define DHTTYPE DHT11
#define SERVO_PIN 13

// ====== Virtual Pin Mapping ======
#define VPIN_VOLTAGE V0
#define VPIN_CURRENT V1
#define VPIN_POWER V2
#define VPIN_TEMPERATURE V3
#define VPIN_HUMIDITY V4
#define VPIN_SERVO_ANGLE V5
#define VPIN_OPERATION_MODE V6
#define VPIN_MANUAL_ANGLE V7
#define VPIN_PREDICTED_POWER V8
#define VPIN_FAULT_ALERT V9
#define VPIN_SYSTEM_STATUS V10
#define VPIN_ENERGY_TODAY V11

// ====== Components ======
Adafruit_INA219 ina219;
DHT dht(DHTPIN, DHTTYPE);

// ====== System State ======
int currentServoAngle = 90;
String operationMode = "auto"; // "auto" or "manual"
float energyToday = 0.0;
unsigned long lastEnergyUpdate = 0;
unsigned long lastSensorRead = 0;
unsigned long lastBlynkUpdate = 0;
unsigned long lastGoogleSheetsUpdate = 0;

// ====== Servo Motor ======
#include <ESP32Servo.h>
Servo panelServo;

// ====== Blynk Widget Handlers ======
BLYNK_WRITE(VPIN_OPERATION_MODE) {
  int modeValue = param.asInt();
  operationMode = (modeValue == 0) ? "auto" : "manual";
  Serial.println("Mode changed to: " + operationMode);
  
  // Send notification
  Blynk.virtualWrite(VPIN_FAULT_ALERT, "Mode set to: " + operationMode);
}

BLYNK_WRITE(VPIN_MANUAL_ANGLE) {
  int angle = param.asInt();
  if (operationMode == "manual") {
    moveServo(angle);
    Serial.println("Manual angle set to: " + String(angle));
  }
}

BLYNK_CONNECTED() {
  Serial.println("Blynk connected!");
  // Sync the operation mode switch on reconnect
  Blynk.virtualWrite(VPIN_OPERATION_MODE, operationMode == "auto" ? 0 : 1);
}

// ====== Setup ======
void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("=== AI PV Tracker Starting ===");

  // Initialize I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.printf("I2C started on SDA=%d SCL=%d\n", I2C_SDA, I2C_SCL);

  // Initialize Servo
  panelServo.attach(SERVO_PIN);
  moveServo(currentServoAngle);
  Serial.println("Servo initialized");

  // Initialize INA219
  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip!");
    while (1) delay(10);
  }
  Serial.println("INA219 found");

  // Initialize DHT
  dht.begin();
  Serial.println("DHT initialized");

  // Connect to WiFi
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    Serial.print('.');
    delay(500);
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi connection failed!");
  }

  // Start Blynk
  Blynk.config(auth);
  Blynk.connect(5000); // 5 second timeout
  
  Serial.println("=== System Ready ===");
}

// ====== Main Loop ======
void loop() {
  Blynk.run();
  
  unsigned long currentTime = millis();
  
  // Read sensors every 5 seconds
  if (currentTime - lastSensorRead >= 5000) {
    readAndProcessSensors();
    lastSensorRead = currentTime;
  }
  
  // Update Blynk every 10 seconds
  if (currentTime - lastBlynkUpdate >= 10000) {
    updateBlynkDashboard();
    lastBlynkUpdate = currentTime;
  }
  
  // Update Google Sheets every 1 minute
  if (currentTime - lastGoogleSheetsUpdate >= 60000) {
    sendToGoogleSheets();
    lastGoogleSheetsUpdate = currentTime;
  }
  
  // Auto mode servo control every 30 seconds
  if (operationMode == "auto" && currentTime % 30000 < 100) {
    adjustPanelForSun();
  }
  
  delay(100); // Small delay to prevent watchdog timeout
}

// ====== Sensor Reading ======
void readAndProcessSensors() {
  // Read INA219
  float busVoltage = ina219.getBusVoltage_V();
  float current_mA = ina219.getCurrent_mA();
  float current_A = current_mA / 1000.0;
  float power_W = busVoltage * current_A;
  
  // Read DHT11
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  
  // Handle sensor errors
  if (isnan(temp) || isnan(hum)) {
    Serial.println("DHT read error");
    temp = -999;
    hum = -999;
  }
  
  // Update energy tracking
  updateEnergyTracking(power_W);
  
  // Debug output
  Serial.printf("V:%.2fV I:%.3fA P:%.2fW T:%.1fC H:%.1f%% Energy:%.1fWh\n", 
                busVoltage, current_A, power_W, temp, hum, energyToday);
}

// ====== Energy Tracking ======
void updateEnergyTracking(float currentPower) {
  unsigned long currentTime = millis();
  if (lastEnergyUpdate > 0) {
    float hoursPassed = (currentTime - lastEnergyUpdate) / 3600000.0;
    energyToday += currentPower * hoursPassed;
  }
  lastEnergyUpdate = currentTime;
  
  // Reset energy at midnight (simplified)
  if (currentTime % 86400000 < 1000) { // Approximately every 24 hours
    energyToday = 0;
  }
}

// ====== Blynk Dashboard Update ======
void updateBlynkDashboard() {
  // Read current sensor values
  float busVoltage = ina219.getBusVoltage_V();
  float current_A = ina219.getCurrent_mA() / 1000.0;
  float power_W = busVoltage * current_A;
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  
  // Update basic sensors
  Blynk.virtualWrite(VPIN_VOLTAGE, busVoltage);
  Blynk.virtualWrite(VPIN_CURRENT, current_A);
  Blynk.virtualWrite(VPIN_POWER, power_W);
  Blynk.virtualWrite(VPIN_TEMPERATURE, temp);
  Blynk.virtualWrite(VPIN_HUMIDITY, hum);
  
  // Update system status
  Blynk.virtualWrite(VPIN_SERVO_ANGLE, currentServoAngle);
  Blynk.virtualWrite(VPIN_ENERGY_TODAY, energyToday);
  Blynk.virtualWrite(VPIN_SYSTEM_STATUS, 1); // 1 = normal
  
  // Simulate AI predictions (replace with actual AI later)
  float predictedPower = simulateAIPrediction();
  Blynk.virtualWrite(VPIN_PREDICTED_POWER, predictedPower);
  
  // Check for faults
  checkForFaults();
}

// ====== Servo Control ======
void moveServo(int angle) {
  angle = constrain(angle, 0, 180);
  panelServo.write(angle);
  currentServoAngle = angle;
  delay(100); // Allow servo to move
}

void adjustPanelForSun() {
  // Simple time-based sun tracking (replace with AI later)
  int hour = (millis() / 3600000) % 24; // Simulated hour
  int optimalAngle = map(hour, 6, 18, 0, 180); // 6am to 6pm
  optimalAngle = constrain(optimalAngle, 0, 180);
  
  if (operationMode == "auto") {
    moveServo(optimalAngle);
    Serial.println("Auto-adjusted to angle: " + String(optimalAngle));
  }
}

// ====== AI Simulation (Replace with actual AI later) ======
float simulateAIPrediction() {
  // Simulate next-day power prediction based on current conditions
  float basePower = ina219.getBusVoltage_V() * (ina219.getCurrent_mA() / 1000.0);
  float temp = dht.readTemperature();
  
  // Simple prediction algorithm
  float prediction = basePower * 6.0; // Estimate daily energy
  if (temp > 35) prediction *= 0.9; // Temperature derating
  if (temp < 15) prediction *= 0.95;
  
  return prediction;
}

// ====== Fault Detection ======
void checkForFaults() {
  float voltage = ina219.getBusVoltage_V();
  float current = ina219.getCurrent_mA() / 1000.0;
  float temp = dht.readTemperature();
  
  String faultMessage = "";
  
  if (voltage < 5.0) {
    faultMessage = "Low voltage detected: " + String(voltage) + "V";
  } else if (voltage > 25.0) {
    faultMessage = "High voltage detected: " + String(voltage) + "V";
  } else if (current > 3.0) {
    faultMessage = "High current detected: " + String(current) + "A";
  } else if (temp > 60.0) {
    faultMessage = "High temperature: " + String(temp) + "C";
  } else if (isnan(temp)) {
    faultMessage = "Temperature sensor error";
  }
  
  if (faultMessage != "") {
    Blynk.virtualWrite(VPIN_FAULT_ALERT, faultMessage);
    Blynk.virtualWrite(VPIN_SYSTEM_STATUS, 0); // 0 = fault
    Serial.println("FAULT: " + faultMessage);
  }
}

// ====== Google Sheets Integration ======
void sendToGoogleSheets() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi offline - skipping Google Sheets");
    return;
  }
  
  HTTPClient http;
  
  float busVoltage = ina219.getBusVoltage_V();
  float current_A = ina219.getCurrent_mA() / 1000.0;
  float power_W = busVoltage * current_A;
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  
  String url = googleScriptURL +
    "?voltage=" + String(busVoltage, 3) +
    "&current=" + String(current_A, 4) +
    "&power=" + String(power_W, 4) +
    "&temp=" + String(temp, 2) +
    "&hum=" + String(hum, 2) +
    "&angle=" + String(currentServoAngle) +
    "&mode=" + operationMode +
    "&energy=" + String(energyToday, 2);
  
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    Serial.println("Google Sheets updated: HTTP " + String(httpCode));
  } else {
    Serial.println("Google Sheets error: " + String(httpCode));
  }
  
  http.end();
}

