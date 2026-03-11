// config.h — Hardware pin definitions and constants
// Nash's DIY Graphing Calculator (Simplified Hardware)
// MCU: Seeed XIAO ESP32S3 | Display: ST7789 320×240 SPI | Input: M5Stack CardKB

#pragma once
#include <Arduino.h>

// ============================================================
//  DISPLAY (ST7789 via SPI) — configured in platformio.ini
// ============================================================
//  MOSI  → GPIO 9   (D10)
//  SCLK  → GPIO 7   (D8)
//  CS    → GPIO 2   (D1)
//  DC    → GPIO 3   (D2)
//  RST   → GPIO 4   (D3)
//  BL    → GPIO 5   (D4)

constexpr int PIN_TFT_BL = 5;

// Screen dimensions (landscape)
constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 240;

// ============================================================
//  CARDKB (M5Stack CardKB via I2C)
// ============================================================
//  SDA   → GPIO 1   (D0)
//  SCL   → GPIO 6   (D5)
//  I2C address: 0x5F

constexpr uint8_t CARDKB_ADDR    = 0x5F;
constexpr uint8_t PIN_I2C_SDA    = 1;
constexpr uint8_t PIN_I2C_SCL    = 6;
constexpr unsigned long CARDKB_POLL_MS = 20;  // Poll interval

// ============================================================
//  BATTERY ADC
// ============================================================
//  Voltage divider on GPIO 10 (D11): 100k/100k, VBAT → ~half
constexpr uint8_t PIN_BATT_ADC   = 10;

// ============================================================
//  WAKE BUTTON (deep sleep wake)
// ============================================================
constexpr uint8_t PIN_WAKE       = 8;   // GPIO 8 (D9)

// ============================================================
//  UI Layout
// ============================================================
constexpr int STATUS_BAR_H   = 22;

// Colors (16-bit 565)
constexpr uint16_t COL_BG         = 0x0000;  // Black
constexpr uint16_t COL_TEXT       = 0xFFFF;  // White
constexpr uint16_t COL_DIM        = 0x7BEF;  // Gray
constexpr uint16_t COL_ACCENT     = 0x07FF;  // Cyan accent
constexpr uint16_t COL_STATUS_BG  = 0x0841;  // Very dark gray
constexpr uint16_t COL_INPUT_BG   = 0x10A2;  // Dark gray
constexpr uint16_t COL_ERROR      = 0xF800;  // Red
