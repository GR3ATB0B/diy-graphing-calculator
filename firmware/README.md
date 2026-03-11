# Nash's DIY Graphing Calculator — Firmware

## Hardware
- **MCU:** Seeed XIAO ESP32S3 (8MB PSRAM)
- **Display:** 2.4" ST7789 IPS 320×240 (SPI, no touch)
- **Input:** 4×4 membrane keypad
- **Power:** LiPo + TP4056

## Pin Mapping

| Function | GPIO |
|----------|------|
| TFT MOSI | 9 |
| TFT SCLK | 7 |
| TFT CS | 2 |
| TFT DC | 3 |
| TFT RST | 4 |
| TFT BL | 5 |
| Keypad Rows | 43, 44, 1, 6 |
| Keypad Cols | 8, 10, 11, 12 |

## Keypad Layout

```
 [1] [2] [3] [+]
 [4] [5] [6] [-]
 [7] [8] [9] [×]
 [⌫] [0] [⏎] [÷]
```

- `⌫` = Backspace (one character)
- `⏎` = Evaluate expression

## Building

```bash
# Install PlatformIO CLI
pip install platformio

# Build
cd firmware/
pio run

# Upload to board
pio run -t upload

# Serial monitor
pio device monitor
```

## Architecture

```
src/
  main.cpp          — Setup + main loop
  display.cpp       — ST7789 driver with sprite framebuffer
  keypad.cpp        — 4×4 matrix scanner with debounce
  screen.cpp        — Stack-based screen manager
  calc_screen.cpp   — Calculator UI + history
  evaluator.cpp     — Recursive descent expression parser

include/
  config.h          — Pin definitions, colors, layout constants
  display.h / keypad.h / screen.h / calc_screen.h / evaluator.h
```

## Expression Evaluator
- Supports: `+`, `-`, `*`, `/`, parentheses, decimals, negative numbers
- Proper operator precedence (PEMDAS)
- Recursive descent parser
