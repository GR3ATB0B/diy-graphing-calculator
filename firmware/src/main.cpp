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
    delay(500);
    Serial.println("Nash Calculator v0.2 (CardKB)");

    Serial.printf("PSRAM size: %d bytes\n", ESP.getPsramSize());

    Serial.println("Display init...");
    display.begin();
    Serial.println("Display OK");

    Serial.println("CardKB init...");
    cardkb.begin();
    Serial.println("CardKB OK");

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

    Serial.println("Setup complete");
}

void loop() {
    char key = cardkb.read();

    if (key != '\0') {
        Serial.printf("Key: 0x%02X '%c'\n", key, key);

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
