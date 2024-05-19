#include <FS.h>
#include <Arduino.h>
#include <arduino-timer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#define AP_NAME "ESPStarterKit"
#define AP_PASSWORD "12345678"

WiFiManager wm;
char hostname[50];
auto timer = timer_create_default();
bool saveConfig = false;

bool toggle_led(void *) {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  return true;
}

void saveConfigCallback () {
  saveConfig = true;
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  if (LittleFS.begin()) {
    if (LittleFS.exists("/config.json")) {
      File configFile = LittleFS.open("/config.json", "r");
      if (configFile) {
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

        JsonDocument json;
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( !deserializeError ) {
          Serial.println("\nparsed json");
          strcpy(hostname, json["hostname"]);
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  }

  WiFiManagerParameter custom_parameter("hostname", "IP/Host", "8.8.8.8", 50);
  WiFiManagerParameter custom_text("<p>This is just a text paragraph</p>");

  wm.addParameter(&custom_parameter);
  wm.addParameter(&custom_text);
  wm.setClass("invert");
  wm.setScanDispPerc(true);
  wm.setSaveConfigCallback(saveConfigCallback);

  if (!wm.autoConnect(AP_NAME, AP_PASSWORD)) {
    delay(5000);
    ESP.restart();
  } else {
    timer.every(1000, toggle_led);

    if (saveConfig) {
      strcpy(hostname, custom_parameter.getValue());
      JsonDocument json;
      json["hostname"] = hostname;
      File configFile = LittleFS.open("/config.json", "w");
      serializeJson(json, configFile);
      configFile.close();
    }

    Serial.println(WiFi.localIP());
  }
}

void loop() {
  timer.tick();
}