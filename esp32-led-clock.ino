#include <WiFi.h>
#include <WiFiClient.h>
#include <Wire.h>
#include "time.h"

const char *ssid = "reboot";
const char *password = "91265264109220132502751234";
const char *ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -18000; // NYC
const int   daylightOffset_sec = 3600;

#define SR_LATCH_GPIO 5
#define SR_CLOCK_GPIO 18
#define SR_DATA_PIN 23
#define ONBOARD_BLUE_LED_GPIO 2
#define SECONDS_LED_GPIO 3
#define PWM_FREQUENCY 5000
#define pwmResolution 8
#define SECONDS_LED_PWM_CHANNEL 0
#define LDR_GPIO 34

// Seven segment display bit mask (active low)
#define DISP_BLANK 10
#define DISP_A 11
#define DISP_C 12
#define DISP_E 13
#define DISP_F 14
#define DISP_P 15

byte table[16] {
  B00000011, B10011111, B00100101, B00001101,
  B10011001, B01001001, B01000001, B00011111,
  B00000001, B00001001, B11111111, B00010001,
  B01100011, B01100001, B01110001, B00110001
};
// -------------------------------------------

int totalFlashcount = 1;

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
  digitalWrite(SR_LATCH_GPIO, LOW);
  int displayDigit = 0;
  switch(totalFlashcount) {
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
      totalFlashcount = 0;
      if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
      }
      break;
    default:
      totalFlashcount = 0;
      break;
  }
  totalFlashcount++;
  
  shiftOut(SR_DATA_PIN, SR_CLOCK_GPIO, LSBFIRST, table[displayDigit]); 
  digitalWrite(SR_LATCH_GPIO, HIGH); 
}

void screenOff() { 
  digitalWrite(SR_LATCH_GPIO, LOW); 
  shiftOut(SR_DATA_PIN, SR_CLOCK_GPIO, LSBFIRST, table[DISP_BLANK]); 
  digitalWrite(SR_LATCH_GPIO, HIGH); 
}

int brightness = 255;

void setBrightness() {
  if (analogRead(LDR_GPIO) < 1200) {
    brightness = 10;
  } else {
    brightness = 255;
  }
}

void readSettings() {
    int address = 0;
    //Serial.print
}

void writeSettings() {
  
}

void setup() {
    Serial.begin(115200);

    pinMode(SR_CLOCK_GPIO, OUTPUT);
    pinMode(SR_LATCH_GPIO, OUTPUT);
    pinMode(SR_DATA_PIN, OUTPUT);
    pinMode(ONBOARD_BLUE_LED_GPIO, OUTPUT);
    ledcSetup(SECONDS_LED_PWM_CHANNEL, PWM_FREQUENCY, pwmResolution);
    ledcAttachPin(SECONDS_LED_GPIO, SECONDS_LED_PWM_CHANNEL);

    //---Connect ESP32 to Wifi Network---
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");
    // Wait for connection
    bool ONBOARD_BLUE_LED_GPIOStatus = HIGH;
    int counter = 0;
    while (WiFi.status() != WL_CONNECTED && counter < 60) {
        delay(500);
        digitalWrite(ONBOARD_BLUE_LED_GPIO, ONBOARD_BLUE_LED_GPIOStatus);
        ONBOARD_BLUE_LED_GPIOStatus = !ONBOARD_BLUE_LED_GPIOStatus;
        counter++;
    }
    if (WiFi.status() != WL_CONNECTED) {
        digitalWrite(ONBOARD_BLUE_LED_GPIO, HIGH);
    } else {
      digitalWrite(ONBOARD_BLUE_LED_GPIO, LOW);
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

void loop() {
    //digitalWrite(SECONDS_LED_GPIO, LOW);
    setBrightness();
    ledcWrite(SECONDS_LED_PWM_CHANNEL, 255 - brightness);
    Display();
    delay(800);
    screenOff();
    delay(200);
    //digitalWrite(SECONDS_LED_GPIO, HIGH);
    ledcWrite(SECONDS_LED_PWM_CHANNEL, 256);
    delay(1000);
}
