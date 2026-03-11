// test_cardkb.ino — Bare-minimum CardKB (M5Stack) I2C test
// Nash's DIY Graphing Calculator
// Target: Seeed XIAO ESP32S3 + M5Stack CardKB
//
// Copy-paste into Arduino IDE. No external libraries needed.
//
// === PIN WIRING ===
// SDA → GPIO 1  (XIAO D0)
// SCL → GPIO 6  (XIAO D5)
// CardKB I2C address: 0x5F

#include <Wire.h>

#define I2C_SDA     1
#define I2C_SCL     6
#define CARDKB_ADDR 0x5F

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("=== CardKB Test ===");

    Wire.begin(I2C_SDA, I2C_SCL);
    Serial.printf("I2C initialized on SDA=%d, SCL=%d\n", I2C_SDA, I2C_SCL);

    // I2C bus scan
    Serial.println("Scanning I2C bus...");
    int found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  Found device at 0x%02X\n", addr);
            found++;
        }
    }
    if (found == 0) {
        Serial.println("  No I2C devices found!");
    } else {
        Serial.printf("  %d device(s) found\n", found);
    }

    Serial.println("Polling CardKB... press keys on the keyboard.");
}

void loop() {
    Wire.requestFrom(CARDKB_ADDR, (uint8_t)1);
    if (Wire.available()) {
        char key = Wire.read();
        if (key != 0) {
            Serial.printf("Key: 0x%02X '%c'\n", key, key);
        }
    }
    delay(20);
}
