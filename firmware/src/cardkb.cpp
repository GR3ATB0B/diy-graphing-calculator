// cardkb.cpp — M5Stack CardKB I2C input driver
// CardKB sends one ASCII byte per key press at address 0x5F.
// Special keys: 0x08=Backspace, 0x0D=Enter, 0x1B=Escape
// Arrow keys: 0xB4=Left, 0xB7=Right, 0xB5=Up, 0xB6=Down

#include "cardkb.h"

void CardKB::begin() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setClock(100000);  // 100kHz I2C

    // Probe for CardKB
    Wire.beginTransmission(CARDKB_ADDR);
    _connected = (Wire.endTransmission() == 0);

    if (_connected) {
        Serial.println("CardKB detected at 0x5F");
    } else {
        Serial.println("CardKB NOT found at 0x5F");
    }
}

char CardKB::read() {
    unsigned long now = millis();
    if (now - _lastPoll < CARDKB_POLL_MS) return '\0';
    _lastPoll = now;

    if (!_connected) return '\0';

    Wire.requestFrom((uint8_t)CARDKB_ADDR, (uint8_t)1);
    if (Wire.available()) {
        char c = Wire.read();
        if (c != 0) return c;  // 0 = no key pressed
    }
    return '\0';
}
