#ifndef LED_H
#define LED_H

#include <Adafruit_NeoPixel.h>

class Led {
  public:
    Led(Adafruit_NeoPixel *strip);
    void setColor(uint32_t color);
    void showAlert(uint32_t color);
    void rainbowAnimation();

  private:
    Adafruit_NeoPixel *_ledStrip;
    uint32_t calculateHalfBrightness(uint32_t originalColor);
    long _firstPixelHue;
    uint8_t _alertState;
};

#endif
