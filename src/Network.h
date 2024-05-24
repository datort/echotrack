#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPping.h>
#include "AsyncPing.h"

#define PING_COUNT 4
#define PING_TIMEOUT 300

class Network {
public:
  Network();

  void setHostname(char* hostname);
  bool updateResponseTime();
  float getResponseTime() const;
  uint8_t getPingCount() const;

private:
  float _responseTime;
  char* _hostname;
  u32_t _pong[PING_COUNT];
  bool _packetLoss;
  AsyncPing _pingJob;
  bool _pingInProgress;
  uint8_t _pingCount;

};

#endif