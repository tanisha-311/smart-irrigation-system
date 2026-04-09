/*
  =============================================
  Smart Irrigation System - Enhanced Version
  Features:
  - Soil moisture, rain, temp & humidity sensing
  - Auto pump control with safety cutoff
  - Temperature-based watering override
  - Buzzer alerts
  - LCD screen cycling (2 pages)
  - Serial data logging (CSV format)
  =============================================
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// ---- PIN DEFINITIONS ----
#define RELAY       12
#define DHTPIN       4
#define DHTTYPE  DHT11
#define RAIN         7
#define SOIL        A0
#define GREEN        8
#define RED         13
#define BUZZER       6   // NEW: Buzzer pin

// ---- THRESHOLDS ----
int   soilThreshold   = 650;
float tempMax         = 40.0;
float humidityMin     = 20.0;
unsigned long pumpMaxRunTime = 10000; // 10 sec max pump run

// ---- OBJECTS ----
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

// ---- STATE VARIABLES ----
unsigned long pumpStartTime  = 0;
bool          pumpRunning    = false;
bool          pumpCutoff     = false;
int           displayPage    = 0;
unsigned long lastDisplaySwitch = 0;
unsigned long lastLog        = 0;
int           wateringCount  = 0;

void beep(int times, int duration = 150) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(duration);
    digitalWrite(BUZZER, LOW);
    delay(100);
  }
}

void setPump(bool on) {
  if (on && !pumpCutoff) {
    if (!pumpRunning) {
      pumpStartTime = millis();
      pumpRunning = true;
      wateringCount++;
      beep(1, 100);
    }
    digitalWrite(RELAY, LOW);
    digitalWrite(GREEN, HIGH);
    digitalWrite(RED, LOW);
  } else {
    if (pumpRunning) pumpRunning = false;
    digitalWrite(RELAY, HIGH);
    digitalWrite(GREEN, LOW);
    digitalWrite(RED, HIGH);
  }
}

void logData(int soil, int rain, float humidity, float temp, String status) {
  Serial.print(millis());   Serial.print(",");
  Serial.print(soil);       Serial.print(",");
  Serial.print(rain);       Serial.print(",");
  Serial.print(humidity);   Serial.print(",");
  Serial.print(temp);       Serial.print(",");
  Serial.println(status);
}

void showSensorPage(int soil, float humidity, float temp) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("S:"); lcd.print(soil);
  lcd.print(" H:"); lcd.print((int)humidity); lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("T:"); lcd.print(temp, 1);
  lcd.print("C W:"); lcd.print(wateringCount);
}

void showStatusPage(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1.substring(0, 16));
  lcd.setCursor(0, 1);
  lcd.print(line2.substring(0, 16));
}

void setup() {
  pinMode(RELAY,  OUTPUT);
  pinMode(RAIN,   INPUT);
  pinMode(GREEN,  OUTPUT);
  pinMode(RED,    OUTPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(RELAY, HIGH);
  digitalWrite(GREEN, LOW);
  digitalWrite(RED,   HIGH);

  lcd.init();
  lcd.backlight();
  dht.begin();
  Serial.begin(9600);

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Smart Irrigation");
  lcd.setCursor(0, 1); lcd.print("  System v2.0   ");
  beep(2, 200);
  delay(2000);

  Serial.println("millis,soil,rain,humidity,temp,status");
}

void loop() {
  int   soil     = analogRead(SOIL);
  int   rain     = digitalRead(RAIN);
  float humidity = dht.readHumidity();
  float temp     = dht.readTemperature();

  if (isnan(humidity)) humidity = 0;
  if (isnan(temp))     temp     = 0;

  String statusLine1 = "";
  String statusLine2 = "";

  if (pumpRunning && (millis() - pumpStartTime >= pumpMaxRunTime)) {
    pumpCutoff = true;
    setPump(false);
    beep(3, 300);
  }

  if (rain == LOW) {
    setPump(false);
    statusLine1 = "Rain detected!";
    statusLine2 = "Pump OFF";
    beep(1, 500);
  } else if (temp > tempMax) {
    setPump(false);
    statusLine1 = "Too hot! " + String(temp, 0) + "C";
    statusLine2 = "Pump OFF";
  } else if (pumpCutoff) {
    setPump(false);
    statusLine1 = "Pump limit hit!";
    statusLine2 = "Safety cutoff";
  } else if (soil > soilThreshold) {
    setPump(true);
    statusLine1 = "Soil dry S:" + String(soil);
    statusLine2 = "Watering...";
  } else {
    setPump(false);
    statusLine1 = "Soil OK  S:" + String(soil);
    statusLine2 = humidity < humidityMin ? "Low humidity!" : "System normal";
  }

  if (humidity < humidityMin && humidity > 0) beep(1, 80);

  if (millis() - lastDisplaySwitch >= 3000) {
    displayPage = 1 - displayPage;
    lastDisplaySwitch = millis();
  }

  if (displayPage == 0) showSensorPage(soil, humidity, temp);
  else                   showStatusPage(statusLine1, statusLine2);

  if (millis() - lastLog >= 5000) {
    String status = pumpRunning ? "WATERING" :
                    (rain == LOW ? "RAIN" :
                    (pumpCutoff  ? "CUTOFF" : "IDLE"));
    logData(soil, rain, humidity, temp, status);
    lastLog = millis();
  }

  delay(2000);
}