#include <Preferences.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <Wire.h>
#include "time.h"
#include "time_zones.h"

Preferences preferences;

String ssid = "";
String pskstr = "";
String location = "";
const char *ntpServer = "pool.ntp.org";

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
    B01001001
};
// -------------------------------------------

int displayDigitIndex = 1;

struct tm timeinfo;

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
            displayDigit = timeinfo.tm_min % 10;
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

void screenWithDotOff() {
    ledcWrite(SECONDS_LED_PWM_CHANNEL, 256);
}

int brightness = 256;

void setBrightness() {
    if (analogRead(GPIO_LDR) < 1200) {
        brightness = 10;
    } else {
        brightness = 256;
    }
    ledcWrite(SECONDS_LED_PWM_CHANNEL, 256 - brightness);
}

void readSettings() {
    preferences.begin("credentials", false);
    ssid = preferences.getString("ssid", "");
    pskstr = preferences.getString("pskstr", "");
    location = preferences.getString("location", "America/New_York");
    preferences.end();
}

void writeSettings() {
    preferences.begin("credentials", false);
    preferences.putString("ssid", ssid);
    preferences.putString("pskstr", pskstr);
    preferences.putString("location", location);
    preferences.end();
}

void displayWifiFailure() {
    writeTodisplayTime(DISP_E);
    bool GPIO_ONBOARD_BLUE_LEDStatus = HIGH;
    for(int i=0; i < 20; i++) {
        digitalWrite(GPIO_ONBOARD_BLUE_LED, GPIO_ONBOARD_BLUE_LEDStatus);
        delay(100);
        GPIO_ONBOARD_BLUE_LEDStatus = !GPIO_ONBOARD_BLUE_LEDStatus;
    }
    digitalWrite(GPIO_ONBOARD_BLUE_LED, HIGH);
}

void connectToWifi() {
    writeTodisplayTime(DISP_S);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pskstr.c_str());
    Serial.println("");
    // Wait for connection
    waitForWifiToConnect(30);
}

void waitForWifiToConnect(int secs) {
    bool GPIO_ONBOARD_BLUE_LEDStatus = HIGH;
    int counter = 0;
    while (WiFi.status() != WL_CONNECTED && counter < (secs * 2)) {
        digitalWrite(GPIO_ONBOARD_BLUE_LED, GPIO_ONBOARD_BLUE_LEDStatus);
        delay(500);
        GPIO_ONBOARD_BLUE_LEDStatus = !GPIO_ONBOARD_BLUE_LEDStatus;
        counter++;
    }
    if (WiFi.status() != WL_CONNECTED) {
        displayWifiFailure();
    } else {
        digitalWrite(GPIO_ONBOARD_BLUE_LED, LOW);
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(GPIO_SR_CLOCK, OUTPUT);
    pinMode(GPIO_SR_LATCH, OUTPUT);
    pinMode(GPIO_SR_DATA, OUTPUT);
    pinMode(GPIO_ONBOARD_BLUE_LED, OUTPUT);
    ledcSetup(SECONDS_LED_PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttachPin(GPIO_SECONDS_LED, SECONDS_LED_PWM_CHANNEL);

    setBrightness();

    // read preferences from memory
    readSettings();

    if (ssid == "" || pskstr == ""){
        Serial.println("No values saved for ssid or psk");
        displayWifiFailure();
    } else {
        //---Connect ESP32 to Wifi Network---
        connectToWifi();
    }

    if (WiFi.status() != WL_CONNECTED) {
        writeTodisplayTime(DISP_C);

        /* Set ESP32 to WiFi Station mode */
        WiFi.mode(WIFI_AP_STA);
        /* start SmartConfig */
        WiFi.beginSmartConfig();

        /* Wait for SmartConfig packet from mobile */
        Serial.println("Waiting for SmartConfig.");
        while (!WiFi.smartConfigDone()) {
            delay(500);
            Serial.print(".");
        }
        Serial.println("");
        ssid = WiFi.SSID();
        pskstr = WiFi.psk();

        Serial.println("SmartConfig done.");
        /* Wait for WiFi to connect to AP */
        Serial.println("Waiting for WiFi");
        waitForWifiToConnect(30);
        if (WiFi.status() == WL_CONNECTED) {
          writeSettings();
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        //displayTime WiFi info to Serial Monitor on successful connection
        Serial.println("");
        Serial.print("Connected to ");
        Serial.println(ssid);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        // ****
        configTimeWithTz(getTzByLocation(location), ntpServer);
        delay(1000);
        printLocalTime();
    }
}

void loop() {
    setBrightness();
    displayTime();
    delay(800);
    screenOff();
    delay(200);
    screenWithDotOff();
    delay(1000);
}
