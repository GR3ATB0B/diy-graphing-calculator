// main.cpp — Nash's DIY Graphing Calculator
// Seeed XIAO ESP32S3 + ILI9341 240×320 + M5Stack CardKB

#include <Arduino.h>
#include "display.h"
#include "cardkb.h"
#include "screen.h"
#include "calc_screen.h"

Display       display;
CardKB        cardkb;
ScreenManager screens;
CalcScreen    calcScreen;

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("Nash Calculator v0.3 (ILI9341)");

    display.begin();
    cardkb.begin();

    screens.push(&calcScreen);

    Screen* s = screens.current();
    if (s) s->draw(display.tft());

    Serial.println("Setup complete");
}

void loop() {
    char key = cardkb.read();

    if (key != '\0') {
        Serial.printf("Key: 0x%02X '%c'\n", key, key);
        Screen* s = screens.current();
        if (s) {
            s->handleInput(key);
            s->draw(display.tft());
        }
    }

    // Periodic redraw for cursor blink
    static unsigned long lastRedraw = 0;
    unsigned long now = millis();
    if (now - lastRedraw > 500) {
        lastRedraw = now;
        Screen* s = screens.current();
        if (s) s->draw(display.tft());
    }

    delay(5);
}
