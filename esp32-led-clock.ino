#include <WiFi.h>
#include <WiFiClient.h>
#include <Wire.h>
#include "time.h"

const char *ssid = "reboot";
const char *password = "91265264109220132502751234";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -18000; // NYC
const int   daylightOffset_sec = 3600;

#define latchPin 5
#define clockPin 18
#define dataPin 23
#define onboardLED 2
#define secondsLED 3
#define pwmFreq 5000
#define pwmResolution 8
#define secondsLEDPWMChannel 0
#define LDRPin 34



byte table[10] {B00000011, B10011111, B00100101, B00001101, B10011001, B01001001, B01000001, B00011111, B00000001, B00001001};
int flashcount = 1;

struct tm timeinfo;

void printLocalTime()
{
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void Display() {
  digitalWrite(latchPin, LOW);
  int displayDigit = 0;
  switch(flashcount) {
    case 1:
      displayDigit = timeinfo.tm_hour / 10;
      break;
    case 2:
      displayDigit = timeinfo.tm_hour % 10;
      break;
    case 3:
      displayDigit = timeinfo.tm_min / 10;
      break;
    case 4:
      displayDigit =timeinfo.tm_min % 10;
      break;
    case 5:
      screenOff();
      flashcount = 0;
      if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
      }
      break;
    default:
      flashcount = 0;
      break;
  }
  flashcount++;
  
  shiftOut(dataPin, clockPin, LSBFIRST, table[displayDigit]); 
  digitalWrite(latchPin, HIGH); 
}

void screenOff() { 
  digitalWrite(latchPin, LOW); 
  shiftOut(dataPin, clockPin, LSBFIRST, B11111111); 
  digitalWrite(latchPin, HIGH); 
}

void setup() {
    Serial.begin(115200);

    pinMode(clockPin, OUTPUT);
    //pinMode(LDRPin, INPUT);
    pinMode(latchPin, OUTPUT);
    pinMode(dataPin, OUTPUT);
    pinMode(onboardLED, OUTPUT);
    ledcSetup(secondsLEDPWMChannel, pwmFreq, pwmResolution);
    ledcAttachPin(secondsLED, secondsLEDPWMChannel);

    //---Connect ESP32 to Wifi Network---
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");
    // Wait for connection
    bool onboardLEDStatus = HIGH;
    int counter = 0;
    while (WiFi.status() != WL_CONNECTED && counter < 60) {
        delay(500);
        digitalWrite(onboardLED, onboardLEDStatus);
        onboardLEDStatus = !onboardLEDStatus;
        counter++;
    }
    if (WiFi.status() != WL_CONNECTED) {
        digitalWrite(onboardLED, HIGH);
    } else {
      digitalWrite(onboardLED, LOW);
      //Display WiFi info to Serial Monitor on successful connection
      Serial.println("");
      Serial.print("Connected to ");
      Serial.println(ssid);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      // ****
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    }
    printLocalTime();
}

int brightness = 255;

void setBrightness() {
  if (analogRead(LDRPin) < 1200) {
    brightness = 10;
  } else {
    brightness = 255;
  }
}

void loop() {
    //digitalWrite(secondsLED, LOW);
    setBrightness();
    ledcWrite(secondsLEDPWMChannel, 255 - brightness);
    Serial.println(analogRead(LDRPin));
    Display();
    delay(800);
    screenOff();
    delay(200);
    //digitalWrite(secondsLED, HIGH);
    ledcWrite(secondsLEDPWMChannel, 256);
    Serial.println(analogRead(LDRPin));
    delay(1000);
    Serial.println(brightness);
}
