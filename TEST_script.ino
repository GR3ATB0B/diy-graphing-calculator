#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <Wire.h>

// TFT pins
#define TFT_CS    2
#define TFT_DC    3
#define TFT_RST   4
#define TFT_MOSI  9
#define TFT_CLK   7
#define TFT_BLK   5

// I2C pins (corrected)
#define KB_SDA    1   // D0
#define KB_SCL    6   // D5
#define KB_ADDR   0x5F

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST);

int cursorX = 10;
int cursorY = 10;
const int LINE_HEIGHT = 20;
const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 320;
const int MARGIN = 10;

void setup() {
  Serial.begin(115200);

  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);

  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);

  Wire.begin(KB_SDA, KB_SCL);

  tft.setCursor(cursorX, cursorY);
  tft.print("Type something:");
  cursorY += LINE_HEIGHT * 2;
  tft.setCursor(cursorX, cursorY);
}

void loop() {
  Wire.requestFrom(KB_ADDR, 1);

  if (Wire.available()) {
    char key = Wire.read();

    if (key != 0) {
      Serial.print("Key pressed: 0x");
      Serial.println(key, HEX);

      if (key == 0x0D) {
        cursorY += LINE_HEIGHT;
        cursorX = MARGIN;
        if (cursorY > SCREEN_HEIGHT - LINE_HEIGHT) {
          tft.fillScreen(ILI9341_BLACK);
          cursorY = MARGIN;
          cursorX = MARGIN;
        }
        tft.setCursor(cursorX, cursorY);

      } else if (key == 0x08) {
        if (cursorX > MARGIN) {
          cursorX -= 12;
          tft.fillRect(cursorX, cursorY, 12, LINE_HEIGHT, ILI9341_BLACK);
          tft.setCursor(cursorX, cursorY);
        }

      } else {
        tft.print(key);
        cursorX += 12;
        if (cursorX > SCREEN_WIDTH - MARGIN - 12) {
          cursorX = MARGIN;
          cursorY += LINE_HEIGHT;
          if (cursorY > SCREEN_HEIGHT - LINE_HEIGHT) {
            tft.fillScreen(ILI9341_BLACK);
            cursorY = MARGIN;
            cursorX = MARGIN;
          }
          tft.setCursor(cursorX, cursorY);
        }
      }
    }
  }
  delay(20);
}