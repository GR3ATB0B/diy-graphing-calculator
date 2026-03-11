// display.h — ILI9341 display driver (Adafruit_GFX, direct drawing)
#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "config.h"

class Display {
public:
    void begin();
    Adafruit_ILI9341& tft() { return _tft; }

private:
    Adafruit_ILI9341 _tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST);
};
