// cardkb.h — M5Stack CardKB input driver over I2C
// Address 0x5F, returns ASCII key codes when a key is pressed
#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "config.h"

class CardKB {
public:
    // Initialize I2C and check if CardKB is present
    void begin();

    // Poll for a key press. Returns '\0' if nothing pressed.
    char read();

    // True if CardKB was detected on boot
    bool isConnected() const { return _connected; }

private:
    bool _connected = false;
    unsigned long _lastPoll = 0;
};
