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

#define CLOCK_ON_BY_DEFAULT true

#define NUM_COLOR_ANIMATIONS 8
uint8_t selectedColorAnimation = 0;
uint32_t colorAnimations[NUM_COLOR_ANIMATIONS] = {
  20000000,
  0xff0000,
  0x00ff00,
  0x0000ff,
  0xffff00,
  0x00ffff,
  0xff00ff,
  0xffffff,
};

WiFiManager wm;
char hostname[128];
char warnlevelOrange[6] = "25";
char warnlevelRed[6] = "40";
char ntpOffset[10] = "7200";
auto timer = timer_create_default();
bool saveConfig = false;
bool setupMenuStarted = false;

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
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", atoi(ntpOffset), 15000);

bool showClock = CLOCK_ON_BY_DEFAULT;
uint8_t lastPingCount;
float lastResponseTime;

bool measureResponseTask(void *) {
  if (showClock) return true;

  network.updateResponseTime();
  return true;
}

int getPingAnimationByte() {
  switch (network.getPingCount()) {
    case 1:
      return 0b00001000;

    case 2: 
      return 0b00001100;

    case 3: 
      return 0b01001100;

    case 4: 
      return 0b01011100;
  }

  return 0b00000000;
}

bool updateDisplayTask(void *) {
  if (showClock) {
    String time = timeClient.getFormattedTime();
    time.replace(":", "");
    display.showString(time.c_str(), 6, 0, 0b01010000);
  } else {
    float responseTime = network.getResponseTime();
    if (responseTime > 0) {
      if (lastPingCount != network.getPingCount()) {
        display.setSegments(getPingAnimationByte(), 0);
        lastPingCount = network.getPingCount();
      }

      if (lastResponseTime != responseTime) {
        display.showNumber(responseTime, 1, 5, 1);
        lastResponseTime = responseTime;
      }
    }
  }

  return true;
}

bool animationTask(void *) {
  uint32_t animationToRun = colorAnimations[selectedColorAnimation];

  if (!showClock && network.getResponseTime() > atoi(warnlevelOrange)) {
    led.showAlert(network.getResponseTime() > atoi(warnlevelRed) ? 0xff0000 : 0xff4d00);
    return true;
  }

  if (animationToRun == 20000000) {
    led.rainbowAnimation();
  } else {
    led.setColor(animationToRun);
  }

  return true;
}

void saveConfigCallback () {
  saveConfig = true;
}

void toggleDisplayInfo() {
  showClock = !showClock;
  display.showString(showClock ? "THE CLOCK" : "PING-CHECK");
}

void toggleColorScheme() {
  if (selectedColorAnimation == NUM_COLOR_ANIMATIONS - 1) selectedColorAnimation = 0;
  else selectedColorAnimation += 1;
}

void startTimers() {
  timer.every(500, measureResponseTask);
  timer.every(100, updateDisplayTask);
  timer.every(125, animationTask);
}

void toggleSetupMenu() {
  if (!setupMenuStarted) {
    timer.cancel();

    display.setScrolldelay(300);
    display.showString(String("Go to " + WiFi.localIP().toString()).c_str());
    display.showString(WiFi.localIP().toString().c_str());
    display.showString("SETUP");

    setupMenuStarted = true;
    wm.startWebPortal();
  } else {
    wm.stopWebPortal();
    setupMenuStarted = false;

    display.showString("DONE");
    startTimers();
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  display.begin();
  display.setBrightness(0);
  display.showString("HELLO");

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
          strcpy(warnlevelOrange, json["warnlevelOrange"]);
          strcpy(warnlevelRed, json["warnlevelRed"]);
          strcpy(ntpOffset, json["ntpOffset"]);
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();

        timeClient.setTimeOffset(atoi(ntpOffset));
      }
    }
  }

  WiFiManagerParameter hostnameParameter("hostname", "IP/Host to ping check", "8.8.8.8", 50);
  WiFiManagerParameter warnlevelOrangeParameter("warnlevelOrange", "Medium warning at (ms)", "25", 3);
  WiFiManagerParameter warnlevelRedParameter("warnlevelRed", "Critical warning at (ms)", "40", 3);
  WiFiManagerParameter ntpOffsetParameter("ntpOffset", "Timezone offset (CEST 7200)", "7200", 5);

  wm.addParameter(&hostnameParameter);
  wm.addParameter(&warnlevelOrangeParameter);
  wm.addParameter(&warnlevelRedParameter);
  wm.addParameter(&ntpOffsetParameter);

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
      strcpy(hostname, hostnameParameter.getValue());
      strcpy(warnlevelOrange, warnlevelOrangeParameter.getValue());
      strcpy(warnlevelRed, warnlevelRedParameter.getValue());
      strcpy(ntpOffset, ntpOffsetParameter.getValue());

      JsonDocument json;
      json["hostname"] = hostname;
      json["warnlevelOrange"] = warnlevelOrange;
      json["warnlevelRed"] = warnlevelRed;
      json["ntpOffset"] = ntpOffset;
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

    btn.attachClick(toggleDisplayInfo);
    btn.attachDoubleClick(toggleColorScheme);
    //btn.attachLongPressStop(toggleSetupMenu);

    startTimers();
  }
}

void loop() {
  timer.tick();
  btn.tick();
  timeClient.update();
}