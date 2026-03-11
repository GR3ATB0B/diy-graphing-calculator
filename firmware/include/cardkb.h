// cardkb.h — M5Stack CardKB input driver over I2C
#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "config.h"

class CardKB {
public:
    void begin();
    char read();
    bool isConnected() const { return _connected; }

private:
    bool _connected = false;
    unsigned long _lastPoll = 0;
};
