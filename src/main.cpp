#include <FS.h>
#include <Arduino.h>
#include <arduino-timer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <OneButton.h>
#include <Adafruit_NeoPixel.h>
#include <TM1637TinyDisplay6.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "Led.h"
#include "Network.h"

#define AP_NAME "EchoTrack Setup AP"
#define AP_PASSWORD "12345678"

#define WARN_LEVEL_MEDIUM 20
#define WARN_LEVEL_CRITICAL 30

#define CLOCK_ON_BY_DEFAULT true


WiFiManager wm;
char hostname[256];
auto timer = timer_create_default();
bool saveConfig = false;

#define CLK D1
#define DIO D2
TM1637TinyDisplay6 display(CLK, DIO);


#define LED_COUNT 8
#define LED_PIN D8
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
Led led(&strip);

#define BUTTON D7
OneButton btn = OneButton(
  BUTTON,
  false,
  false
);

Network network = Network();

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 7200, 30000);

bool showClock = CLOCK_ON_BY_DEFAULT;

bool measureResponseTask(void *) {
  if (showClock) return true;

  network.updateResponseTime();
  return true;
}

bool updateDisplayTask(void *) {
  if (showClock) {
    String time = timeClient.getFormattedTime();
    time.replace(":", "");
    display.showString(time.c_str(), 6, 0, 0b01010000);
  } else {
    float responseTime = network.getResponseTime();
    if (responseTime > 0) {
      display.showNumber(responseTime, 1);
    }
  }

  return true;
}

bool animationTask(void *) {
  led.rainbowAnimation();

  return true;
}

void saveConfigCallback () {
  saveConfig = true;
}

void toggleDisplayInfo() {
  showClock = !showClock;
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  display.begin();
  display.setBrightness(0);
  display.showString("START");

  if (LittleFS.begin()) {
    if (LittleFS.exists("/config.json")) {
      File configFile = LittleFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("Found config.json");
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

        JsonDocument json;
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if (!deserializeError) {
          Serial.println("\nparsed json");
          strcpy(hostname, json["hostname"]);
          Serial.print("Setting hostname to: ");
          Serial.println(hostname);
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  }

  WiFiManagerParameter hostname_text("<p>The host to Ping check:</p>");
  WiFiManagerParameter hostname_parameter("hostname", "IP/Host", "8.8.8.8", 50);

  wm.addParameter(&hostname_text);
  wm.addParameter(&hostname_parameter);
  wm.setClass("invert");
  wm.setScanDispPerc(true);
  wm.setSaveConfigCallback(saveConfigCallback);

  if (!wm.getWiFiIsSaved()) {
    display.showString("SETUP");
  }

  led.setColor(0x0000ff);

  if (!wm.autoConnect(AP_NAME, AP_PASSWORD)) {
    delay(5000);
    ESP.restart();
  } else {
    Serial.println(saveConfig);
    if (saveConfig) {
      Serial.println(hostname_parameter.getValue());
      strcpy(hostname, hostname_parameter.getValue());
      Serial.println(hostname);
      JsonDocument json;
      json["hostname"] = hostname;
      File configFile = LittleFS.open("/config.json", "w");
      serializeJson(json, configFile);
      configFile.close();
      Serial.println("Saved config to config.json");
    }

    Serial.println(WiFi.localIP());

    led.setColor(0x00ffff);
    network.setHostname(hostname);

    timeClient.begin();
    timeClient.update();

    timer.every(500, measureResponseTask);
    timer.every(200, updateDisplayTask);
    timer.every(50, animationTask);

    btn.attachClick(toggleDisplayInfo);
  }
}

void loop() {
  timer.tick();
  btn.tick();
  timeClient.update();
}