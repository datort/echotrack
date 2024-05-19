#include "Led.h"

Led::Led(Adafruit_NeoPixel *strip) : _ledStrip(strip), _brightness(21), _direction(1), _firstPixelHue(0l) {
  _ledStrip->begin();
  _ledStrip->setBrightness(75);
  _ledStrip->fill(0xff0000);
  _ledStrip->show();
}

void Led::setColor(uint32_t color) {
  _ledStrip->fill(color);
  _ledStrip->show();
}

void Led::rainbowAnimation() {
  if (_firstPixelHue >= 5*65536) _firstPixelHue = 0;

  for (int i = 0; i < _ledStrip->numPixels(); i++) { 
    int pixelHue = _firstPixelHue + (i * 65536L / _ledStrip->numPixels());
    _ledStrip->setPixelColor(i, _ledStrip->gamma32(_ledStrip->ColorHSV(pixelHue)));
  }

  _ledStrip->show();
  _firstPixelHue += 250;
}
