// display.cpp — ST7789 display driver
#include "display.h"

void Display::begin() {
    _tft.init();
    _tft.setRotation(1);  // Landscape: 320×240
    _tft.fillScreen(COL_BG);

    // Log PSRAM availability
    if (ESP.getPsramSize() > 0) {
        Serial.printf("PSRAM available: %d bytes\n", ESP.getPsramSize());
    } else {
        Serial.println("PSRAM not available");
    }

    // Try to create full-screen sprite as framebuffer
    void* result = _sprite.createSprite(SCREEN_W, SCREEN_H);
    if (result != nullptr) {
        _sprite.setColorDepth(16);
        _sprite.fillSprite(COL_BG);
        _useSprite = true;
        Serial.println("Sprite framebuffer created OK");
    } else {
        _useSprite = false;
        Serial.println("Sprite creation failed — falling back to direct drawing");
    }

    // Backlight on
    pinMode(PIN_TFT_BL, OUTPUT);
    setBacklight(255);
}

void Display::flush() {
    if (_useSprite) {
        _sprite.pushSprite(0, 0);
    }
    // If direct drawing, nothing to flush — draws go straight to TFT
}

void Display::setBacklight(uint8_t level) {
    analogWrite(PIN_TFT_BL, level);
}
