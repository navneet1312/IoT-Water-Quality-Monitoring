/*
 * River & Lake Water Quality Monitoring System (v2)
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
 * 
 * Changes in v2:
 * - Added humidity threshold checking (HUMIDITY_MIN / HUMIDITY_MAX)
 * - Fixed buzzer blocking issue that caused Wokwi simulation to freeze
 * - Updated LCD and Serial output to show humidity status
 * - Added TEST_MODE: set to true to use hardcoded values instead of potentiometers
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
#define HUMIDITY_MIN   30.0    // % Relative Humidity
#define HUMIDITY_MAX   90.0    // % Relative Humidity

// ====== TEST MODE ======
// Set to true to use the hardcoded values below instead of reading potentiometers.
// Set to false to read live sensor/potentiometer values from the circuit.
#define TEST_MODE      true

// ====== Objects ======
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHTesp dht;

// ====== Variables (these are the default/test values) ======
// When TEST_MODE is true, these values are used directly.
// Change them to simulate different water conditions.
float phValue = 7.0;           // Safe range: 6.5 - 8.5
float turbidityValue = 25.0;   // Safe range: 0 - 50 NTU
float tdsValue = 200.0;        // Safe range: 0 - 500 ppm
float temperature = 25.0;      // Safe range: 5 - 35 °C
float humidity = 60.0;         // Safe range: 30 - 90 %
bool waterSafe = true;
unsigned long lastUpload = 0;
const unsigned long uploadInterval = 15000; // 15 seconds
int displayMode = 0;
unsigned long lastDisplaySwitch = 0;
unsigned long lastBuzzerToggle = 0;
bool buzzerOn = false;

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
  Serial.println("  IoT-Based with Cloud Integration v2");
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
  #if TEST_MODE
    // TEST MODE: Skip sensor reads, use the hardcoded values from the Variables section.
    // Change the values at the top of the file to simulate different conditions.
    Serial.println("[TEST MODE] Using hardcoded sensor values");
    return;
  #endif

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
  if (humidity < HUMIDITY_MIN || humidity > HUMIDITY_MAX) {
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
      lcd.print("Humid: ");
      lcd.print(humidity, 0);
      lcd.print("% ");
      lcd.print(humidity >= HUMIDITY_MIN && humidity <= HUMIDITY_MAX ? "OK" : "!");
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
    buzzerOn = false;
  } else {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    // Non-blocking buzzer alarm pattern (fixes Wokwi freeze)
    if (millis() - lastBuzzerToggle >= 500) {
      lastBuzzerToggle = millis();
      if (buzzerOn) {
        noTone(BUZZER_PIN);
        buzzerOn = false;
      } else {
        tone(BUZZER_PIN, 1000);
        buzzerOn = true;
      }
    }
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
  Serial.print(" % [");
  Serial.print(humidity >= HUMIDITY_MIN && humidity <= HUMIDITY_MAX ? "Normal" : "ABNORMAL");
  Serial.println("]");
  
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
