#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <MAX30105.h>
#include "heartRate.h"

#define WIFI_SSID "SM-A50"
#define WIFI_PASSWORD "12345678"
#define FIREBASE_HOST "https://smart-care-hub-6519a-default-rtdb.firebaseio.com/"

MAX30105 particleSensor;
const int LED_PIN = 8;

const byte RATE_SIZE = 4; 
byte rates[RATE_SIZE]; 
byte rateSpot = 0;
long lastBeat = 0; 

float beatsPerMinute;
int beatAvg;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  Serial.begin(115200);
  delay(1000);

  Wire.begin(0, 1);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX3010x not found. Check wiring.");
    while (1) {
      digitalWrite(LED_PIN, LOW); delay(100);
      digitalWrite(LED_PIN, HIGH); delay(1000);
    }
  }
  Serial.println("MAX3010x sensor found!");

  particleSensor.setup(50, 4, 2, 100, 411, 4096);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  digitalWrite(LED_PIN, HIGH); delay(100);
  digitalWrite(LED_PIN, LOW); delay(1000);
}

void loop() {
  long irValue = particleSensor.getIR();

  if (irValue > 50000) {

    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 210000.0 / delta;

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
    
    if(WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      String url = String(FIREBASE_HOST) + "/wristBand/heartRate.json";
      http.begin(url);
      http.addHeader("Content-Type", "application/json");
      String body = String(beatAvg);
      int httpResponseCode = http.PUT(body);
      http.end();

      digitalWrite(LED_PIN, LOW); delay(2000);
      digitalWrite(LED_PIN, HIGH); delay(2000);
    }
    digitalWrite(LED_PIN, LOW); delay(100);
    digitalWrite(LED_PIN, HIGH); delay(900);
  } else {
    
    Serial.println("Please place finger on sensor.");
    digitalWrite(LED_PIN, LOW); delay(50);
    digitalWrite(LED_PIN, HIGH); delay(950);
  }
}