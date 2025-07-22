#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define WIFI_SSID "SM-A50"
#define WIFI_PASSWORD "12345678"

#define FIREBASE_HOST "https://smart-care-hub-6519a-default-rtdb.firebaseio.com/"

// Reed switch
const int REED_PIN_SLOT1 = 0;
const int REED_PIN_SLOT2 = 14;
const int REED_PIN_SLOT3 = 12;

const unsigned long debounceDelay = 50;
unsigned long lastDebounceTime1 = 0, lastDebounceTime2 = 0, lastDebounceTime3 = 0;

int prevReedState1 = HIGH;
int prevReedState2 = HIGH;
int prevReedState3 = HIGH;

String lastTakenSlot = "None";
String nextDoseSlot = "Slot1";

void updateFirebaseStatus(const String& slotName, int status) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;
    String url = String(FIREBASE_HOST) + "pillBox/" + slotName + "/status.json";

    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    String payload = "{\"status\":" + String(status) + "}";

    int httpCode = http.PUT(payload);

    if (httpCode > 0) {
      Serial.printf("Updated %s to Firebase. HTTP code: %d\n", slotName.c_str(), httpCode);
      Serial.println(http.getString());
    } else {
      Serial.printf("Failed to update %s. HTTP code: %d\n", slotName.c_str(), httpCode);
      Serial.println(http.errorToString(httpCode));
    }
    http.end();
  } else {
    Serial.println("WiFi not connected. Firebase update skipped.");
  }
}

String getNextSlot(const String& currentSlot) {
  if (currentSlot == "Slot1") return "Slot2";
  else if (currentSlot == "Slot2") return "Slot3";
  else return "Slot1";
}

void updateOLED() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Last taken: ");
  display.println(lastTakenSlot);
  display.print("Next dose: ");
  display.println(nextDoseSlot);
  display.display();
}

void handleSlot(int pin, int& prevState, unsigned long& lastDebounceTime, const String& slotName) {
  int reading = digitalRead(pin);

  if (reading != prevState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != prevState) {
      prevState = reading;  // Move this here (before checking value)

      if (reading == HIGH) {  // Opened
        Serial.println(slotName + " opened!");
        updateFirebaseStatus(slotName, 0);
        lastTakenSlot = slotName;
        nextDoseSlot = getNextSlot(slotName);
        updateOLED();
      } else {  // Closed
        Serial.println(slotName + " closed!");
        updateFirebaseStatus(slotName, 1);
      }
    }
  }
}

void connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);

 if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 failed"));
    while (true);
  }
  display.display();
  delay(2000);
  display.clearDisplay();

  pinMode(REED_PIN_SLOT1, INPUT_PULLUP);
  pinMode(REED_PIN_SLOT2, INPUT_PULLUP);
  pinMode(REED_PIN_SLOT3, INPUT_PULLUP);

  connectToWiFi();

  updateFirebaseStatus("Slot1", digitalRead(REED_PIN_SLOT1));
  updateFirebaseStatus("Slot2", digitalRead(REED_PIN_SLOT2));
  updateFirebaseStatus("Slot3", digitalRead(REED_PIN_SLOT3));

  updateOLED();

}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  handleSlot(REED_PIN_SLOT1, prevReedState1, lastDebounceTime1, "Slot1");
  handleSlot(REED_PIN_SLOT2, prevReedState2, lastDebounceTime2, "Slot2");
  handleSlot(REED_PIN_SLOT3, prevReedState3, lastDebounceTime3, "Slot3");

  delay(1000);
}
