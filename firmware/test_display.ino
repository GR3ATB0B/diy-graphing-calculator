// test_display.ino — Bare-minimum ST7789 display test
// Nash's DIY Graphing Calculator
// Target: Seeed XIAO ESP32S3 + ST7789 320×240 SPI
//
// Copy-paste into Arduino IDE. Requires TFT_eSPI library.
// In TFT_eSPI User_Setup.h, set USER_SETUP_LOADED or use this sketch's defines.
//
// === PIN WIRING ===
// TFT_MOSI  → GPIO 9   (XIAO D10)
// TFT_SCLK  → GPIO 7   (XIAO D8)
// TFT_CS    → GPIO 2   (XIAO D1)
// TFT_DC    → GPIO 3   (XIAO D2)
// TFT_RST   → GPIO 4   (XIAO D3)
// TFT_BL    → GPIO 5   (XIAO D4)  — Backlight (PWM)
// SPI Speed → 40 MHz (safe for breadboard)

// ---- TFT_eSPI configuration (override library defaults) ----
#define USER_SETUP_LOADED 1
#define ST7789_DRIVER     1
#define TFT_WIDTH         240
#define TFT_HEIGHT        320
#define TFT_RGB_ORDER     TFT_RGB
#define TFT_MOSI          9
#define TFT_SCLK          7
#define TFT_CS            2
#define TFT_DC            3
#define TFT_RST           4
#define SPI_FREQUENCY     40000000
#define SPI_READ_FREQUENCY 20000000
#define LOAD_GLCD         1
#define LOAD_FONT2        1
#define LOAD_FONT4        1
#define SMOOTH_FONT       1

#define PIN_BL 5

#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("=== Display Test ===");

    // Backlight on
    pinMode(PIN_BL, OUTPUT);
    analogWrite(PIN_BL, 255);

    Serial.println("Initializing TFT...");
    tft.init();
    tft.setRotation(1);  // Landscape 320×240
    Serial.println("TFT initialized");

    // Red
    Serial.println("Fill RED");
    tft.fillScreen(TFT_RED);
    delay(1000);

    // Green
    Serial.println("Fill GREEN");
    tft.fillScreen(TFT_GREEN);
    delay(1000);

    // Blue
    Serial.println("Fill BLUE");
    tft.fillScreen(TFT_BLUE);
    delay(1000);

    // Text
    Serial.println("Drawing text: HELLO NASH");
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);  // Middle-center
    tft.setTextSize(3);
    tft.drawString("HELLO NASH", 160, 120);

    Serial.println("=== Display Test Complete ===");
}

void loop() {
    // Nothing to do
    delay(1000);
}
