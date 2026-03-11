// config.h — Hardware pin definitions and constants
// Nash's DIY Graphing Calculator
// MCU: Seeed XIAO ESP32S3 | Display: ILI9341 240×320 SPI | Input: M5Stack CardKB

#pragma once
#include <Arduino.h>

// ============================================================
//  DISPLAY (ILI9341 via software SPI)
// ============================================================
#define TFT_CS    2
#define TFT_DC    3
#define TFT_RST   4
#define TFT_MOSI  9
#define TFT_CLK   7
#define TFT_BLK   5

// Screen dimensions (landscape, rotation 1)
constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 240;

// ============================================================
//  CARDKB (M5Stack CardKB via I2C)
// ============================================================
#define KB_SDA    1
#define KB_SCL    6
#define KB_ADDR   0x5F

constexpr unsigned long CARDKB_POLL_MS = 20;

// ============================================================
//  UI Layout
// ============================================================
constexpr int STATUS_BAR_H = 20;

// Colors (16-bit 565)
constexpr uint16_t COL_BG         = 0x0000;  // Black
constexpr uint16_t COL_TEXT       = 0xFFFF;  // White
constexpr uint16_t COL_DIM        = 0x7BEF;  // Gray
constexpr uint16_t COL_ACCENT     = 0x07FF;  // Cyan
constexpr uint16_t COL_STATUS_BG  = 0x0841;  // Very dark gray
constexpr uint16_t COL_ERROR      = 0xF800;  // Red
