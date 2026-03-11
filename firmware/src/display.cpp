// display.cpp — ST7789 display driver
#include "display.h"

void Display::begin() {
    _tft.init();
    _tft.setRotation(1);  // Landscape: 320×240
    _tft.fillScreen(COL_BG);

    // Create full-screen sprite as framebuffer
    _sprite.createSprite(SCREEN_W, SCREEN_H);
    _sprite.setColorDepth(16);
    _sprite.fillSprite(COL_BG);

    // Backlight on
    pinMode(PIN_TFT_BL, OUTPUT);
    setBacklight(255);
}

void Display::flush() {
    _sprite.pushSprite(0, 0);
}

void Display::setBacklight(uint8_t level) {
    analogWrite(PIN_TFT_BL, level);
}
