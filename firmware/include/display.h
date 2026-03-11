// display.h — ST7789 display driver with sprite-based framebuffer
#pragma once
#include <TFT_eSPI.h>
#include "config.h"

class Display {
public:
    void begin();

    // Access the sprite (framebuffer) for drawing
    TFT_eSprite& sprite() { return _sprite; }
    TFT_eSPI&    tft()    { return _tft; }

    // Push sprite framebuffer to display
    void flush();

    // Set backlight (0-255)
    void setBacklight(uint8_t level);

private:
    TFT_eSPI    _tft = TFT_eSPI();
    TFT_eSprite _sprite = TFT_eSprite(&_tft);
};
