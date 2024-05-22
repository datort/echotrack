#include "Led.h"

Led::Led(Adafruit_NeoPixel *strip) : _ledStrip(strip), _firstPixelHue(0l) {
  _ledStrip->begin();
  _ledStrip->setBrightness(75);
  _ledStrip->fill(0xff0000);
  _ledStrip->show();
}

void Led::setColor(uint32_t color) {
  _ledStrip->fill(color);
  _ledStrip->show();
}

void Led::showAlert(uint32_t color) {
  _ledStrip->fill(_alertState == 1 ? calculateHalfBrightness(color) : color);
  _ledStrip->show();
  
  _alertState = _alertState == 1 ? 0 : 1;
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

uint32_t Led::calculateHalfBrightness(uint32_t originalColor) {
  uint8_t red = (originalColor >> 16) & 0xFF;
  uint8_t green = (originalColor >> 8) & 0xFF;
  uint8_t blue = originalColor & 0xFF;

  uint8_t redHalf = red * 0.5;
  uint8_t greenHalf = green * 0.5;
  uint8_t blueHalf = blue * 0.5;

  return (redHalf << 16) | (greenHalf << 8) | blueHalf;
}