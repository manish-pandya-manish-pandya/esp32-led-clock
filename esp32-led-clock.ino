#include <WiFi.h>
#include <WiFiClient.h>
#include <Wire.h>
#include "time.h"

char *ssid = "reboot";
char *password = "91265264109220132502751234";
const char *ntpServer = "pool.ntp.org";
long  gmtOffset_sec = -18000; // NYC
int   daylightOffset_sec = 3600;

#define GPIO_SR_LATCH 5
#define GPIO_SR_CLOCK 18
#define GPIO_SR_DATA 23

#define GPIO_ONBOARD_BLUE_LED 2

#define GPIO_LDR 34
#define GPIO_SPARE_GPIO 17

#define GPIO_SECONDS_LED 3

#define SECONDS_LED_PWM_CHANNEL 0
#define PWM_FREQUENCY 5000
#define PWM_RESOLUTION 8

// Seven segment display bit mask (active low)
#define DISP_BLANK 10
#define DISP_A 11
#define DISP_C 12
#define DISP_E 13
#define DISP_F 14
#define DISP_P 15
#define DISP_S 16

byte table[17] {
  B00000011, B10011111, B00100101, B00001101,
  B10011001, B01001001, B01000001, B00011111,
  B00000001, B00001001, B11111111, B00010001,
  B01100011, B01100001, B01110001, B00110001,
  B01001000
};
// -------------------------------------------

int displayDigitIndex = 1;

struct tm timeinfo;

boolean isSetupMode = false;

void printLocalTime()
{
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void writeTodisplayTime(int index) {
  digitalWrite(GPIO_SR_LATCH, LOW);
  shiftOut(GPIO_SR_DATA, GPIO_SR_CLOCK, LSBFIRST, table[index]); 
  digitalWrite(GPIO_SR_LATCH, HIGH);  
}

void displayTime() {
  int displayDigit = 0;
  switch(displayDigitIndex) {
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
      displayDigit = DISP_BLANK;
      displayDigitIndex = 0;
      if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
      }
      break;
    default:
      displayDigitIndex = 0;
      break;
  }
  displayDigitIndex++;
  writeTodisplayTime(displayDigit);
}

void screenOff() {
  writeTodisplayTime(DISP_BLANK);
}

int brightness = 255;

void setBrightness() {
  if (analogRead(GPIO_LDR) < 1200) {
    brightness = 10;
  } else {
    brightness = 255;
  }
  ledcWrite(SECONDS_LED_PWM_CHANNEL, 255 - brightness);
}

void readSettings() {
    int address = 0;
    //Serial.print
}

void writeSettings() {
  
}

void setup() {
    Serial.begin(115200);

    pinMode(GPIO_SR_CLOCK, OUTPUT);
    pinMode(GPIO_SR_LATCH, OUTPUT);
    pinMode(GPIO_SR_DATA, OUTPUT);
    pinMode(GPIO_ONBOARD_BLUE_LED, OUTPUT);
    ledcSetup(SECONDS_LED_PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttachPin(GPIO_SECONDS_LED, SECONDS_LED_PWM_CHANNEL);

    //---Connect ESP32 to Wifi Network---
    setBrightness();
    writeTodisplayTime(DISP_S);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");
    // Wait for connection
    bool GPIO_ONBOARD_BLUE_LEDStatus = HIGH;
    int counter = 0;
    while (WiFi.status() != WL_CONNECTED && counter < 60) {
        delay(500);
        digitalWrite(GPIO_ONBOARD_BLUE_LED, GPIO_ONBOARD_BLUE_LEDStatus);
        GPIO_ONBOARD_BLUE_LEDStatus = !GPIO_ONBOARD_BLUE_LEDStatus;
        counter++;
    }
    if (WiFi.status() != WL_CONNECTED) {
        writeTodisplayTime(DISP_E);
        for(int i=0; i<10; i++) {
          digitalWrite(GPIO_ONBOARD_BLUE_LED, HIGH);
          delay(100);         
          digitalWrite(GPIO_ONBOARD_BLUE_LED, LOW);
          delay(100);         
        }
        isSetupMode = true;
        WiFi.begin("reboot", password); 
    } else {
      digitalWrite(GPIO_ONBOARD_BLUE_LED, LOW);
      //displayTime WiFi info to Serial Monitor on successful connection
      Serial.println("");
      Serial.print("Connected to ");
      Serial.println(ssid);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      // ****
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      printLocalTime();
    }
}

void loop() {
    if (isSetupMode) {
        writeTodisplayTime(DISP_C);
        digitalWrite(GPIO_ONBOARD_BLUE_LED, HIGH);
        delay(100);         
        digitalWrite(GPIO_ONBOARD_BLUE_LED, LOW);
        delay(100);         
        digitalWrite(GPIO_ONBOARD_BLUE_LED, HIGH);
        delay(400);         
        digitalWrite(GPIO_ONBOARD_BLUE_LED, LOW);
        delay(400);
        if (WiFi.status() == WL_CONNECTED) {
          isSetupMode = false;
          //displayTime WiFi info to Serial Monitor on successful connection
          Serial.println("");
          Serial.print("Connected to ");
          Serial.println(ssid);
          Serial.print("IP address: ");
          Serial.println(WiFi.localIP());
          // ****
          configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
          printLocalTime();
        }
    } else {
        setBrightness();
        displayTime();
        delay(800);
        screenOff();
        delay(200);
        ledcWrite(SECONDS_LED_PWM_CHANNEL, 256);
        delay(1000);
  }
}
