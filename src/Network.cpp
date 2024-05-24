#include "Network.h"

Network::Network() : _responseTime(0.0f) {
}

bool Network::updateResponseTime() {
  if (_pingInProgress) return true;
  if (_hostname[0] == '\0') {
    Serial.println("No hostname configured - don't know what to ping");
    return false;
  }

  _pingInProgress = true;

  IPAddress addr;

  if (!WiFi.hostByName(_hostname, addr))
    addr.fromString(_hostname);
  
  for (int i = 0; i < PING_COUNT; i++) {
    _pong[i] = 99;
  }
  _packetLoss = false;

  _pingJob.on(true, [this](const AsyncPingResponse& response) {
    //IPAddress addr(response.addr);

    _pingCount += 1;
    if (response.answer) {
      _pong[response.icmp_seq - 1] = response.time;
      //Serial.printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%d ms\n", response.size, addr.toString().c_str(), response.icmp_seq, response.ttl, response.time);
    } else {
      _packetLoss = true;
      //Serial.printf("no answer yet for %s icmp_seq=%d\n", addr.toString().c_str(), response.icmp_seq);
    }
      
    return false;
  });

  _pingJob.on(false, [this](const AsyncPingResponse& response) {
    IPAddress addr(response.addr);

    u32_t total = 0;
    for (int i = 0; i < PING_COUNT; i++) {
      total += _pong[i];
    }

    _responseTime = (float)total/PING_COUNT;
    _pingInProgress = false;
    _pingCount = 0;

    return true;
  });

  _pingJob.begin(addr, PING_COUNT, PING_TIMEOUT);

  return true;
}

void Network::setHostname(char* hostname) {
  _hostname = hostname;
}

float Network::getResponseTime() const {
  return _responseTime > 0 ? _responseTime : -1;
}

uint8_t Network::getPingCount() const {
  return _pingCount;
}
