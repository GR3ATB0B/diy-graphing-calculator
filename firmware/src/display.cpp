// display.cpp — ILI9341 display driver
#include "display.h"

void Display::begin() {
    // Backlight on
    pinMode(TFT_BLK, OUTPUT);
    digitalWrite(TFT_BLK, HIGH);

    _tft.begin();
    _tft.setRotation(0);  // Portrait 240×320
    _tft.fillScreen(COL_BG);

    Serial.println("ILI9341 display initialized (240x320 portrait)");
}
