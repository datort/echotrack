#ifndef LED_H
#define LED_H

#include <Adafruit_NeoPixel.h>

class Led {
  public:
    Led(Adafruit_NeoPixel *strip);
    void setColor(uint32_t color);
    void rainbowAnimation();

  private:
    Adafruit_NeoPixel *_ledStrip;
    long _firstPixelHue;
    int _brightness;
    int _direction;
};

#endif
