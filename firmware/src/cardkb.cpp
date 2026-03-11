// cardkb.cpp — M5Stack CardKB I2C input driver
#include "cardkb.h"

void CardKB::begin() {
    Wire.begin(KB_SDA, KB_SCL);
    Wire.setClock(100000);

    Wire.beginTransmission(KB_ADDR);
    _connected = (Wire.endTransmission() == 0);

    Serial.println(_connected ? "CardKB detected at 0x5F" : "CardKB NOT found");
}

char CardKB::read() {
    unsigned long now = millis();
    if (now - _lastPoll < CARDKB_POLL_MS) return '\0';
    _lastPoll = now;
    if (!_connected) return '\0';

    Wire.requestFrom((uint8_t)KB_ADDR, (uint8_t)1);
    if (Wire.available()) {
        char c = Wire.read();
        if (c != 0) return c;
    }
    return '\0';
}
