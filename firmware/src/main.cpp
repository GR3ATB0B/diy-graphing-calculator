// main.cpp — Nash's DIY Graphing Calculator
// Seeed XIAO ESP32S3 + ST7789 320×240 + M5Stack CardKB

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
    Serial.println("Nash Calculator v0.2 (CardKB)");

    display.begin();
    cardkb.begin();

    // Wake button (for future deep sleep wake)
    pinMode(PIN_WAKE, INPUT_PULLUP);

    // Battery ADC
    pinMode(PIN_BATT_ADC, INPUT);

    // Push the calculator as the home screen
    screens.push(&calcScreen);

    // Initial draw
    Screen* s = screens.current();
    if (s) {
        s->draw(display.sprite());
        display.flush();
    }
}

void loop() {
    char key = cardkb.read();

    if (key != '\0') {
        Screen* s = screens.current();
        if (s) {
            s->handleInput(key);
            s->draw(display.sprite());
            display.flush();
        }
    }

    // Redraw periodically for cursor blink
    static unsigned long lastRedraw = 0;
    unsigned long now = millis();
    if (now - lastRedraw > 500) {
        lastRedraw = now;
        Screen* s = screens.current();
        if (s) {
            s->draw(display.sprite());
            display.flush();
        }
    }

    delay(5);
}
