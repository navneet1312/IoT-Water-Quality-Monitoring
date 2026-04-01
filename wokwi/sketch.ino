/*
 * River & Lake Water Quality Monitoring System
 * IoT-Based Real-Time Water Monitoring with Cloud Integration
 * 
 * Components:
 * - ESP32 DevKit V1 (Microcontroller with WiFi)
 * - DHT22 (Water Temperature Sensor)
 * - Potentiometer 1 (Simulating pH Sensor)
 * - Potentiometer 2 (Simulating Turbidity Sensor)  
 * - Potentiometer 3 (Simulating TDS Sensor)
 * - 16x2 I2C LCD Display
 * - Green LED (Safe Water Indicator)
 * - Red LED (Danger/Alert Indicator)
 * - Buzzer (Alarm for Critical Levels)
 * 
 * Cloud: ThingSpeak IoT Platform
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHTesp.h"

// ====== Pin Definitions ======
#define DHT_PIN        4       // DHT22 Data Pin
#define PH_PIN         34      // pH Sensor (Potentiometer)
#define TURBIDITY_PIN  35      // Turbidity Sensor (Potentiometer)
#define TDS_PIN        32      // TDS Sensor (Potentiometer)
#define GREEN_LED      25      // Safe Water LED
#define RED_LED        26      // Danger LED
#define BUZZER_PIN     27      // Alarm Buzzer

// ====== WiFi Configuration ======
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ====== ThingSpeak Configuration ======
const char* thingspeakServer = "http://api.thingspeak.com/update";
const char* apiKey = "69YEUKNDNXPZ914S";  // ThingSpeak Write API Key

// ====== Sensor Thresholds ======
#define PH_MIN         6.5
#define PH_MAX         8.5
#define TURBIDITY_MAX  50.0    // NTU (Nephelometric Turbidity Units)
#define TDS_MAX        500.0   // ppm (Parts Per Million)
#define TEMP_MIN       5.0     // °C
#define TEMP_MAX       35.0    // °C

// ====== Objects ======
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHTesp dht;

// ====== Variables ======
float phValue = 7.0;
float turbidityValue = 0.0;
float tdsValue = 0.0;
float temperature = 25.0;
float humidity = 60.0;
bool waterSafe = true;
unsigned long lastUpload = 0;
const unsigned long uploadInterval = 15000; // 15 seconds
int displayMode = 0;
unsigned long lastDisplaySwitch = 0;

// ====== Custom LCD Characters ======
byte waterDrop[8] = {
  0b00100,
  0b00100,
  0b01110,
  0b01110,
  0b11111,
  0b11111,
  0b01110,
  0b00000
};

byte checkMark[8] = {
  0b00000,
  0b00001,
  0b00011,
  0b10110,
  0b11100,
  0b01000,
  0b00000,
  0b00000
};

byte dangerSign[8] = {
  0b00100,
  0b01110,
  0b01010,
  0b01110,
  0b01110,
  0b00100,
  0b00100,
  0b00000
};

void setup() {
  Serial.begin(115200);
  Serial.println("========================================");
  Serial.println("  River & Lake Water Monitoring System");
  Serial.println("  IoT-Based with Cloud Integration");
  Serial.println("========================================");
  
  // Initialize pins
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, waterDrop);
  lcd.createChar(1, checkMark);
  lcd.createChar(2, dangerSign);
  
  // Splash screen
  lcd.setCursor(0, 0);
  lcd.write(0);
  lcd.print(" Water Monitor");
  lcd.setCursor(0, 1);
  lcd.print("  IoT System v2");
  
  // Initialize DHT22
  dht.setup(DHT_PIN, DHTesp::DHT22);
  
  // Connect WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed - running offline");
  }
  
  delay(2000);
  lcd.clear();
  
  // Initial LED state
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
}

void loop() {
  // ====== Read All Sensors ======
  readSensors();
  
  // ====== Analyze Water Quality ======
  analyzeWaterQuality();
  
  // ====== Update Display (Rotating) ======
  updateDisplay();
  
  // ====== Control Indicators ======
  controlIndicators();
  
  // ====== Print to Serial Monitor ======
  printSerialData();
  
  // ====== Upload to ThingSpeak Cloud ======
  if (millis() - lastUpload >= uploadInterval) {
    uploadToCloud();
    lastUpload = millis();
  }
  
  delay(2000);
}

void readSensors() {
  // Read DHT22 Temperature & Humidity
  TempAndHumidity data = dht.getTempAndHumidity();
  if (!isnan(data.temperature)) {
    temperature = data.temperature;
  }
  if (!isnan(data.humidity)) {
    humidity = data.humidity;
  }
  
  // Read pH Sensor (Potentiometer simulated: 0-4095 -> 0-14 pH)
  int phRaw = analogRead(PH_PIN);
  phValue = map(phRaw, 0, 4095, 0, 1400) / 100.0;
  
  // Read Turbidity Sensor (Potentiometer simulated: 0-4095 -> 0-100 NTU)
  int turbRaw = analogRead(TURBIDITY_PIN);
  turbidityValue = map(turbRaw, 0, 4095, 0, 10000) / 100.0;
  
  // Read TDS Sensor (Potentiometer simulated: 0-4095 -> 0-1000 ppm)
  int tdsRaw = analogRead(TDS_PIN);
  tdsValue = map(tdsRaw, 0, 4095, 0, 100000) / 100.0;
}

void analyzeWaterQuality() {
  waterSafe = true;
  
  if (phValue < PH_MIN || phValue > PH_MAX) {
    waterSafe = false;
  }
  if (turbidityValue > TURBIDITY_MAX) {
    waterSafe = false;
  }
  if (tdsValue > TDS_MAX) {
    waterSafe = false;
  }
  if (temperature < TEMP_MIN || temperature > TEMP_MAX) {
    waterSafe = false;
  }
}

void updateDisplay() {
  if (millis() - lastDisplaySwitch >= 3000) {
    displayMode = (displayMode + 1) % 4;
    lastDisplaySwitch = millis();
    lcd.clear();
  }
  
  switch (displayMode) {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("pH: ");
      lcd.print(phValue, 1);
      lcd.print("  ");
      lcd.print(phValue >= PH_MIN && phValue <= PH_MAX ? "OK" : "!");
      lcd.setCursor(0, 1);
      lcd.print("Turb: ");
      lcd.print(turbidityValue, 1);
      lcd.print(" NTU");
      break;
      
    case 1:
      lcd.setCursor(0, 0);
      lcd.print("TDS: ");
      lcd.print(tdsValue, 0);
      lcd.print(" ppm");
      lcd.setCursor(0, 1);
      lcd.print("Temp: ");
      lcd.print(temperature, 1);
      lcd.print((char)223);
      lcd.print("C");
      break;
      
    case 2:
      lcd.setCursor(0, 0);
      lcd.print("Humidity: ");
      lcd.print(humidity, 0);
      lcd.print("%");
      lcd.setCursor(0, 1);
      if (waterSafe) {
        lcd.write(1);
        lcd.print(" Water: SAFE");
      } else {
        lcd.write(2);
        lcd.print(" Water: DANGER");
      }
      break;
      
    case 3:
      lcd.setCursor(0, 0);
      lcd.write(0);
      lcd.print(" Water Quality");
      lcd.setCursor(0, 1);
      lcd.print("Monitor Active");
      lcd.print(waterSafe ? " " : "!");
      break;
  }
}

void controlIndicators() {
  if (waterSafe) {
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    noTone(BUZZER_PIN);
  } else {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    // Alarm pattern
    tone(BUZZER_PIN, 1000, 200);
    delay(300);
    noTone(BUZZER_PIN);
  }
}

void printSerialData() {
  Serial.println("\n--- Water Quality Report ---");
  Serial.print("pH Level:     ");
  Serial.print(phValue, 2);
  Serial.print(" [");
  Serial.print(phValue >= PH_MIN && phValue <= PH_MAX ? "Normal" : "ABNORMAL");
  Serial.println("]");
  
  Serial.print("Turbidity:    ");
  Serial.print(turbidityValue, 2);
  Serial.print(" NTU [");
  Serial.print(turbidityValue <= TURBIDITY_MAX ? "Normal" : "HIGH");
  Serial.println("]");
  
  Serial.print("TDS:          ");
  Serial.print(tdsValue, 0);
  Serial.print(" ppm [");
  Serial.print(tdsValue <= TDS_MAX ? "Normal" : "HIGH");
  Serial.println("]");
  
  Serial.print("Temperature:  ");
  Serial.print(temperature, 1);
  Serial.print(" °C [");
  Serial.print(temperature >= TEMP_MIN && temperature <= TEMP_MAX ? "Normal" : "ABNORMAL");
  Serial.println("]");
  
  Serial.print("Humidity:     ");
  Serial.print(humidity, 1);
  Serial.println(" %");
  
  Serial.print("Water Status: ");
  Serial.println(waterSafe ? "✅ SAFE" : "⚠️ DANGER - ALERT!");
  Serial.println("----------------------------");
}

void uploadToCloud() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    String url = String(thingspeakServer);
    url += "?api_key=" + String(apiKey);
    url += "&field1=" + String(phValue, 2);
    url += "&field2=" + String(turbidityValue, 2);
    url += "&field3=" + String(tdsValue, 0);
    url += "&field4=" + String(temperature, 1);
    url += "&field5=" + String(humidity, 1);
    url += "&field6=" + String(waterSafe ? 1 : 0);
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      Serial.println("[CLOUD] Data uploaded to ThingSpeak successfully!");
      Serial.print("[CLOUD] Response code: ");
      Serial.println(httpCode);
    } else {
      Serial.print("[CLOUD] Upload failed. Error: ");
      Serial.println(http.errorToString(httpCode));
    }
    
    http.end();
  } else {
    Serial.println("[CLOUD] WiFi not connected - skipping upload");
  }
}
