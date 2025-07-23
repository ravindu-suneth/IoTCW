#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>

#include <ESP8266Firebase.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define WIFI_SSID "SM-A50"          
#define WIFI_PASSWORD "12345678"    
#define FIREBASE_URL "smart-care-hub-6519a-default-rtdb.firebaseio.com" 

Firebase firebase(FIREBASE_URL);

const int REED_PIN_SLOT2 = 14;  // D5 on NodeMCU
const int REED_PIN_SLOT3 = 12;  // D6 on NodeMCU

bool lastSlot2State = true;
bool lastSlot3State = true;

String lastTaken = "None";
String nextDose = "Slot 2";    // Initial next dose

void updateDisplay();

void setup() {
  Serial.begin(115200);

  pinMode(REED_PIN_SLOT2, INPUT_PULLUP);
  pinMode(REED_PIN_SLOT3, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); 
  }
  display.clearDisplay();
  display.setTextSize(2); 
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Pillbox Monitor");
  display.display();
  delay(2000);

  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  updateDisplay();
}

void loop() {
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    delay(1000); 
    return; 
  }

  bool currentSlot2State = digitalRead(REED_PIN_SLOT2);
  bool currentSlot3State = digitalRead(REED_PIN_SLOT3);

  // Slot 2 opened
  if (lastSlot2State == HIGH && currentSlot2State == LOW) {
    lastTaken = "Slot 2";
    nextDose = "Slot 3";
    Serial.println("Slot 2 opened");
    updateDisplay();
        if (firebase.setInt("pillBox/Slot2/status", 0)) { 
      Serial.println("Slot 2 status updated to 0 in Firebase.");
    } else {
      Serial.println("Failed to update Slot 2 status."); 
    }
    delay(1000); 
  }

  // Slot 3 opened  
  if (lastSlot3State == HIGH && currentSlot3State == LOW) {
    lastTaken = "Slot 3";
    nextDose = "Slot 2";
    Serial.println("Slot 3 opened");
    updateDisplay();
      if (firebase.setInt("pillBox/Slot3/status", 0)) { 
      Serial.println("Slot 3 status updated to 0 in Firebase.");
    } else {
      Serial.println("Failed to update Slot 3 status."); 
    }
    delay(1000); 
  }

  lastSlot2State = currentSlot2State;
  lastSlot3State = currentSlot3State;

  delay(100); 
}

void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(2);
  //display.println("Pillbox Tracker");
  //display.println(); 
  display.print("Last taken: ");
  display.println(lastTaken);
  display.print("Next dose: ");
  display.println(nextDose);
  display.display();
}
