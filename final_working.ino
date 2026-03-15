#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "SPI.h"
#include <TFT_eSPI.h>

// ================================================================
//                     TFT DISPLAY
// ================================================================
TFT_eSPI tft = TFT_eSPI();

// ================================================================
//                     WiFi SETTINGS
// ================================================================
const char* ssid     = "Airtel_Tech";
const char* password = "Tech@2025";

// ================================================================
//                     MQTT SETTINGS
// ================================================================
const char* mqtt_server = "98.130.28.156";
const int   mqtt_port   = 1883;
const char* mqtt_user   = "moambulance";
const char* mqtt_pass   = "P@$sw0rd2001";
const char* mqtt_topic  = "indigurd";

// -------- DHT11 --------
#define DHTPIN 5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// -------- Flame Sensor --------
#define FLAME_PIN 14

// -------- Smoke Sensor --------
#define SMOKE_PIN 34
int sensorThres           = 15;
int smokeConsecutiveCount = 0;
#define SMOKE_CONFIRM_COUNT 3

// -------- Smoke Calibration --------
#define SMOKE_WARMUP_MS 60000UL
bool smokeWarmedUp             = false;
unsigned long smokeWarmupStart = 0;

// -------- Buzzer --------
#define BUZZER_PIN 12

// -------- MPU6050 --------
Adafruit_MPU6050 mpu;
float prevMagnitude      = 0;
float vibrationThreshold = 2.0;
bool firstReading        = true;

// -------- Timers --------
unsigned long buzzerStartTime  = 0;
bool buzzerActive              = false;
unsigned long lastLoopTime     = 0;
unsigned long alertScreenStart = 0;
bool alertScreenActive         = false;
#define LOOP_INTERVAL  2000
#define ALERT_DURATION 5000

// -------- WiFi & MQTT --------
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ================================================================
//                     COLORS
// ================================================================
#define C_BG          TFT_BLACK
#define C_HEADER_BG   0x000F
#define C_TITLE       TFT_YELLOW
#define C_COLLEGE     TFT_WHITE
#define C_DEPT        0x07FF
#define C_CARD_BG     0x1082
#define C_CARD_BORDER 0x4208
#define C_LABEL       0xC618
#define C_OK          0x07E0
#define C_ALERT       0xF800
#define C_WARN        0xFFE0
#define C_VALUE       TFT_WHITE
#define C_FOOTER_BG   0x000F

// ================================================================
//                     DRAW HEADER
// ================================================================
void drawHeader() {
  tft.fillRect(0, 0, 240, 70, C_HEADER_BG);

  tft.setTextColor(C_TITLE, C_HEADER_BG);
  tft.setTextSize(3);
  tft.setCursor(30, 8);
  tft.print("InduGuard");

  tft.setTextColor(C_COLLEGE, C_HEADER_BG);
  tft.setTextSize(1);
  tft.setCursor(12, 40);
  tft.print("BJB Autonomous College");

  tft.setTextColor(C_DEPT, C_HEADER_BG);
  tft.setTextSize(1);
  tft.setCursor(55, 55);
  tft.print("Dept: IMSc");

  tft.drawFastHLine(0, 70, 240, C_DEPT);
}

// ================================================================
//                     DRAW CARD
// ================================================================
void drawCard(int x, int y, int w, int h,
              const char* label, const char* value,
              uint16_t valueColor) {

  tft.fillRoundRect(x, y, w, h, 5, C_CARD_BG);
  tft.drawRoundRect(x, y, w, h, 5, C_CARD_BORDER);

  tft.setTextColor(C_LABEL, C_CARD_BG);
  tft.setTextSize(1);
  tft.setCursor(x + 6, y + 6);
  tft.print(label);

  tft.setTextColor(valueColor, C_CARD_BG);
  tft.setTextSize(1);
  tft.setCursor(x + 6, y + 20);
  tft.print(value);
}

// ================================================================
//                     DRAW FOOTER
// ================================================================
void drawFooter(bool wifiOk, bool mqttOk) {
  tft.fillRect(0, 282, 240, 38, C_FOOTER_BG);
  tft.drawFastHLine(0, 282, 240, C_DEPT);

  tft.setTextSize(1);

  tft.fillCircle(14, 295, 5, wifiOk ? C_OK : C_ALERT);
  tft.setTextColor(C_COLLEGE, C_FOOTER_BG);
  tft.setCursor(24, 291);
  tft.print(wifiOk ? "WiFi: Connected" : "WiFi: Disconnected");

  tft.fillCircle(14, 312, 5, mqttOk ? C_OK : C_ALERT);
  tft.setTextColor(C_COLLEGE, C_FOOTER_BG);
  tft.setCursor(24, 308);
  tft.print(mqttOk ? "MQTT: Connected" : "MQTT: Disconnected");
}

// ================================================================
//                     FULL SCREEN ALERT (5 seconds)
// ================================================================
void drawAlertScreen(bool flame, bool smoke, bool vibration) {
  tft.fillScreen(C_ALERT);

  tft.setTextColor(TFT_WHITE, C_ALERT);
  tft.setTextSize(4);
  tft.setCursor(80, 30);
  tft.print("!!");

  tft.setTextSize(2);
  tft.setCursor(40, 90);
  tft.print("ALERT ACTIVE");

  tft.drawFastHLine(10, 120, 220, TFT_WHITE);

  int yPos = 140;
  tft.setTextSize(2);

  if (flame) {
    tft.setTextColor(C_WARN, C_ALERT);
    tft.setCursor(20, yPos);
    tft.print("FLAME DETECTED");
    yPos += 35;
  }
  if (smoke) {
    tft.setTextColor(C_WARN, C_ALERT);
    tft.setCursor(20, yPos);
    tft.print("SMOKE DETECTED");
    yPos += 35;
  }
  if (vibration) {
    tft.setTextColor(C_WARN, C_ALERT);
    tft.setCursor(20, yPos);
    tft.print("VIBRATION DETECTED");
    yPos += 35;
  }

  tft.drawFastHLine(10, 268, 220, TFT_WHITE);

  tft.setTextColor(TFT_WHITE, C_ALERT);
  tft.setTextSize(1);
  tft.setCursor(50, 276);
  tft.print("Returning in 5 sec...");

  tft.setTextColor(C_WARN, C_ALERT);
  tft.setTextSize(1);
  tft.setCursor(70, 300);
  tft.print("-- InduGuard --");
}

// ================================================================
//                     UPDATE MAIN DASHBOARD
// ================================================================
void updateDisplay(bool flame, bool smoke, bool vibration,
                   float temperature, float humidity,
                   int smokeRaw, bool warmingUp, int warmupSec) {

  // ---- Flame + Smoke ----
  drawCard(3, 74, 115, 52,
           "FLAME",
           flame ? "!! DETECTED !!" : "Not Detected",
           flame ? C_ALERT : C_OK);

  if (warmingUp) {
    char wLabel[22];
    snprintf(wLabel, sizeof(wLabel), "SMOKE  %ds", warmupSec);
    drawCard(122, 74, 115, 52,
             wLabel, "Warming Up...", C_WARN);
  } else {
    char sVal[20];
    snprintf(sVal, sizeof(sVal), "%s (%d)",
             smoke ? "DETECTED" : "Not Detected", smokeRaw);
    drawCard(122, 74, 115, 52,
             "SMOKE", sVal,
             smoke ? C_ALERT : C_OK);
  }

  // ---- Vibration ----
  drawCard(3, 130, 234, 44,
           "VIBRATION",
           vibration ? "!! VIBRATION DETECTED !!" : "No Vibration",
           vibration ? C_ALERT : C_OK);

  // ---- Temp + Humidity ----
  char tempStr[14];
  char humStr[14];

  if (!isnan(temperature)) snprintf(tempStr, sizeof(tempStr), "%.1f C",  temperature);
  else                      snprintf(tempStr, sizeof(tempStr), "Read Error");

  if (!isnan(humidity))    snprintf(humStr,  sizeof(humStr),  "%.1f %%", humidity);
  else                     snprintf(humStr,  sizeof(humStr),  "Read Error");

  drawCard(3,   178, 115, 52, "TEMPERATURE", tempStr,
           isnan(temperature) ? C_ALERT : C_VALUE);
  drawCard(122, 178, 115, 52, "HUMIDITY",    humStr,
           isnan(humidity)    ? C_ALERT : C_VALUE);

  // ---- Status Bar ----
  if (flame || smoke || vibration) {
    tft.fillRect(3, 234, 234, 44, C_ALERT);
    tft.drawRoundRect(3, 234, 234, 44, 5, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, C_ALERT);
    tft.setTextSize(2);
    tft.setCursor(22, 248);
    tft.print("!! ALERT ACTIVE !!");
  } else {
    tft.fillRect(3, 234, 234, 44, 0x0320);
    tft.drawRoundRect(3, 234, 234, 44, 5, C_OK);
    tft.setTextColor(C_OK, 0x0320);
    tft.setTextSize(2);
    tft.setCursor(35, 248);
    tft.print("All Systems OK");
  }
}

// ================================================================
//                     SPLASH SCREEN
// ================================================================
void drawSplash() {
  tft.fillScreen(C_BG);

  tft.setTextColor(C_TITLE, C_BG);
  tft.setTextSize(3);
  tft.setCursor(30, 60);
  tft.print("InduGuard");

  tft.setTextColor(C_COLLEGE, C_BG);
  tft.setTextSize(1);
  tft.setCursor(12, 105);
  tft.print("BJB Autonomous College");

  tft.setTextColor(C_DEPT, C_BG);
  tft.setCursor(55, 120);
  tft.print("Dept: IMSc");

  tft.drawFastHLine(20, 145, 200, C_DEPT);

  tft.setTextColor(C_WARN, C_BG);
  tft.setTextSize(1);
  tft.setCursor(30, 165);
  tft.print("Initializing System...");

  tft.setTextColor(C_LABEL, C_BG);
  tft.setCursor(15, 185);
  tft.print("Smoke sensor warming up 60s");
  tft.setCursor(40, 200);
  tft.print("Other sensors: Active");
}

// ================================================================
//                     WiFi CONNECT
// ================================================================
void connectWiFi() {
  Serial.print("Connecting to WiFi");

  tft.setTextColor(C_WARN, C_BG);
  tft.setTextSize(1);
  tft.setCursor(30, 230);
  tft.print("Connecting to WiFi...");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  tft.fillRect(0, 225, 240, 15, C_BG);
  tft.setTextColor(C_OK, C_BG);
  tft.setCursor(50, 230);
  tft.print("WiFi Connected!");

  Serial.println("\nWiFi Connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// ================================================================
//                     MQTT CONNECT
// ================================================================
void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT...");

    tft.setTextColor(C_WARN, C_BG);
    tft.setTextSize(1);
    tft.setCursor(30, 248);
    tft.print("Connecting to MQTT...");

    if (mqttClient.connect("ESP32_InduGuard", mqtt_user, mqtt_pass)) {
      Serial.println("MQTT Connected!");

      tft.fillRect(0, 243, 240, 15, C_BG);
      tft.setTextColor(C_OK, C_BG);
      tft.setCursor(55, 248);
      tft.print("MQTT Connected!");

    } else {
      Serial.print("Failed rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Retry 3s...");
      delay(3000);
    }
  }
}

// ================================================================
//                     BUZZER HELPER
// ================================================================
void triggerBuzzer() {
  digitalWrite(BUZZER_PIN, HIGH);
  buzzerStartTime = millis();
  buzzerActive    = true;
}

// ================================================================
//                     SETUP
// ================================================================
void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(C_BG);
  drawSplash();

  Wire.begin(21, 22);

  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    tft.setTextColor(C_ALERT, C_BG);
    tft.setTextSize(1);
    tft.setCursor(30, 270);
    tft.print("MPU6050 NOT FOUND!");
    while (1);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  dht.begin();
  pinMode(FLAME_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Start smoke warmup — non-blocking
  smokeWarmupStart = millis();
  Serial.println("Smoke Sensor Warming Up (60s)...");

  connectWiFi();
  mqttClient.setServer(mqtt_server, mqtt_port);
  connectMQTT();

  // Draw main screen
  tft.fillScreen(C_BG);
  drawHeader();
  drawFooter(true, true);

  Serial.println("InduGuard — System Ready");
}

// ================================================================
//                     LOOP
// ================================================================
void loop() {

  // -------- Maintain MQTT --------
  if (!mqttClient.connected()) connectMQTT();
  mqttClient.loop();

  // -------- Buzzer Timer --------
  if (buzzerActive && millis() - buzzerStartTime >= 5000) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
  }

  // -------- Alert Screen Timer --------
  if (alertScreenActive && millis() - alertScreenStart >= ALERT_DURATION) {
    alertScreenActive = false;
    // Redraw full main screen
    tft.fillScreen(C_BG);
    drawHeader();
    drawFooter(WiFi.status() == WL_CONNECTED, mqttClient.connected());
  }

  // -------- Smoke Warmup --------
  int warmupRemaining = 0;
  if (!smokeWarmedUp) {
    unsigned long elapsed = millis() - smokeWarmupStart;
    if (elapsed >= SMOKE_WARMUP_MS) {
      smokeWarmedUp = true;
      Serial.println("Smoke Sensor Ready!");
    } else {
      warmupRemaining = (SMOKE_WARMUP_MS - elapsed) / 1000;
      static unsigned long lastWarmupPrint = 0;
      if (millis() - lastWarmupPrint >= 10000) {
        lastWarmupPrint = millis();
        Serial.print("Smoke Warming Up: ");
        Serial.print(warmupRemaining);
        Serial.println("s remaining");
      }
    }
  }

  // ================================================================
  //   ✅ MPU6050 — OUTSIDE 2s block for INSTANT vibration response
  // ================================================================
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float x = a.acceleration.x;
  float y = a.acceleration.y;
  float z = a.acceleration.z;

  float magnitude        = sqrt(x*x + y*y + z*z);
  bool vibrationDetected = false;

  if (firstReading) {
    prevMagnitude = magnitude;
    firstReading  = false;
  } else {
    vibrationDetected = (abs(magnitude - prevMagnitude) > vibrationThreshold);
  }
  prevMagnitude = magnitude;

  // ✅ Instant vibration response — no 2s delay
  if (vibrationDetected) {
    Serial.println("VIBRATION: Detected!");
    triggerBuzzer();

    // Instant MQTT publish
    StaticJsonDocument<128> vdoc;
    vdoc["vibration"] = "Vibration Detected";
    char vBuf[128];
    serializeJson(vdoc, vBuf);
    mqttClient.publish(mqtt_topic, vBuf);

    // Instant alert screen
    if (!alertScreenActive) {
      alertScreenActive = true;
      alertScreenStart  = millis();
      drawAlertScreen(false, false, true);
    }
  }

  // -------- Non-blocking 2s Interval --------
  if (millis() - lastLoopTime < LOOP_INTERVAL) return;
  lastLoopTime = millis();

  // ================================================================
  //              REMAINING SENSORS — Every 2 seconds
  // ================================================================

  // -------- Flame --------
  bool flameDetected = (digitalRead(FLAME_PIN) == LOW);

  // -------- Smoke --------
  int analogSensor   = analogRead(SMOKE_PIN);
  bool smokeDetected = false;

  if (!smokeWarmedUp) {
    smokeConsecutiveCount = 0;
  } else {
    if (analogSensor > sensorThres) {
      smokeConsecutiveCount++;
      Serial.print("Smoke Count: ");
      Serial.print(smokeConsecutiveCount);
      Serial.print("/");
      Serial.println(SMOKE_CONFIRM_COUNT);
      if (smokeConsecutiveCount >= SMOKE_CONFIRM_COUNT) smokeDetected = true;
    } else {
      smokeConsecutiveCount = 0;
    }
  }

  // -------- DHT11 --------
  float Humidity    = dht.readHumidity();
  float Temperature = dht.readTemperature();

  // ================================================================
  //                     SERIAL MONITOR
  // ================================================================
  Serial.println("======= InduGuard =======");
  Serial.println(flameDetected     ? "FLAME     : Detected!"     : "FLAME     : Not Detected");
  Serial.println(!smokeWarmedUp    ? "SMOKE     : Warming Up..."
               : smokeDetected     ? "SMOKE     : Detected!"     : "SMOKE     : Not Detected");
  Serial.println(vibrationDetected ? "VIBRATION : Detected!"     : "VIBRATION : Not Detected");
  Serial.print  ("Smoke Raw : ");   Serial.println(analogSensor);
  Serial.print  ("Temp      : ");   Serial.print(Temperature); Serial.println(" C");
  Serial.print  ("Humidity  : ");   Serial.print(Humidity);    Serial.println(" %");
  Serial.print  ("Accel X: ");      Serial.print(x);
  Serial.print  (" Y: ");           Serial.print(y);
  Serial.print  (" Z: ");           Serial.println(z);
  Serial.println("-------------------------");

  // ================================================================
  //             DISPLAY — Alert screen OR normal dashboard
  // ================================================================
  bool anyAlert = flameDetected || smokeDetected || vibrationDetected;

  if (anyAlert && !alertScreenActive) {
    alertScreenActive = true;
    alertScreenStart  = millis();
    drawAlertScreen(flameDetected, smokeDetected, vibrationDetected);

  } else if (!alertScreenActive) {
    updateDisplay(flameDetected, smokeDetected, vibrationDetected,
                  Temperature, Humidity,
                  analogSensor, !smokeWarmedUp, warmupRemaining);
    drawFooter(WiFi.status() == WL_CONNECTED, mqttClient.connected());
  }

  // ================================================================
  //                  FULL MQTT JSON — Every 2 seconds
  // ================================================================
  StaticJsonDocument<256> doc;
  doc["flame"]       = flameDetected     ? "Flame Detected"     : "Flame Not Detected";
  doc["smoke"]       = !smokeWarmedUp    ? "Sensor Warming Up"
                     : smokeDetected     ? "Smoke Detected"     : "Smoke Not Detected";
  doc["vibration"]   = vibrationDetected ? "Vibration Detected" : "Vibration Not Detected";
  doc["temperature"] = !isnan(Temperature) ? Temperature : 0;
  doc["humidity"]    = !isnan(Humidity)    ? Humidity    : 0;

  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);
  mqttClient.publish(mqtt_topic, jsonBuffer);

  Serial.print("Published: ");
  Serial.println(jsonBuffer);

  // ================================================================
  //                     BUZZER — Flame & Smoke
  // ================================================================
  if (flameDetected || smokeDetected) triggerBuzzer();
}