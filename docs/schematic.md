# Nash Calculator — Hardware Schematic (v3)

> XIAO ESP32S3 + ILI9341 SPI Display + M5Stack CardKB + TP4056 Charging

---

## System Block Diagram

```
                    ┌─────────────┐
                    │   LiPo      │
                    │  2000mAh    │
                    └──────┬──────┘
                           │
                    ┌──────┴──────┐
                    │   TP4056    │←── USB-C (charge)
                    │  Charger   │
                    └──────┬──────┘
                           │ VBAT (3.7-4.2V)
                    ┌──────┴──────┐
                    │  3.3V LDO   │ (XIAO onboard)
                    └──────┬──────┘
                           │ 3.3V
          ┌────────────────┼────────────────┐
          │                │                │
   ┌──────┴──────┐  ┌─────┴──────┐  ┌─────┴─────────┐
   │  ILI9341    │  │ XIAO       │  │ M5Stack       │
   │  Display    │  │ ESP32S3    │  │ CardKB        │
   │  240×320    │  │            │  │ (I2C 0x5F)    │
   │  (SPI)      │  └────────────┘  └───────────────┘
   └─────────────┘
```

---

## Pin Assignment Table

### XIAO ESP32S3 Pinout

| XIAO Pin | GPIO | Function       | Connected To         |
|----------|------|----------------|----------------------|
| D0       | 1    | I2C SDA        | CardKB SDA           |
| D1       | 2    | SPI CS (TFT)   | ILI9341 CS           |
| D2       | 3    | TFT DC         | ILI9341 DC/RS        |
| D3       | 4    | TFT RST        | ILI9341 RESET        |
| D4       | 5    | TFT Backlight  | ILI9341 BL           |
| D5       | 6    | I2C SCL        | CardKB SCL           |
| D8       | 7    | SPI SCLK       | ILI9341 SCK          |
| D9       | 8    | Wake Button    | Deep sleep wake      |
| D10      | 9    | SPI MOSI       | ILI9341 SDA/MOSI     |
| D11      | 10   | Battery ADC    | VBAT voltage divider |
| D6       | 43   | (free)         | —                    |
| D7       | 44   | (free)         | —                    |

### SPI Bus (ILI9341 Display)

| Signal | XIAO Pin | GPIO | ILI9341 Pin |
|--------|----------|------|-------------|
| MOSI   | D10      | 9    | SDA        |
| SCLK   | D8       | 7    | SCK        |
| CS     | D1       | 2    | CS         |
| DC     | D2       | 3    | DC         |
| RST    | D3       | 4    | RES        |
| BL     | D4       | 5    | BLK        |
| VCC    | 3V3      | —    | VCC (3.3V) |
| GND    | GND      | —    | GND        |

### I2C Bus (CardKB)

| Signal | XIAO Pin | GPIO | Notes                    |
|--------|----------|------|--------------------------|
| SDA    | D0       | 1    | 4.7kΩ pull-up to 3.3V   |
| SCL    | D5       | 6    | 4.7kΩ pull-up to 3.3V   |

Devices on I2C bus:
- M5Stack CardKB: Address `0x5F` (fixed)

### M5Stack CardKB Wiring

| CardKB Pin | Connected To        |
|-----------|---------------------|
| SDA       | GPIO 1 (I2C SDA)    |
| SCL       | GPIO 6 (I2C SCL)    |
| VCC       | 3.3V                |
| GND       | GND                 |

CardKB sends ASCII key codes over I2C:
- Printable chars: 0x20–0x7E (space through ~)
- Backspace: 0x08
- Enter: 0x0D
- Escape: 0x1B
- Arrow keys: Left=0xB4, Right=0xB7, Up=0xB5, Down=0xB6

---

## Power Circuit

### Battery + Charging

```
USB-C ──┬── XIAO ESP32S3 (programming/serial)
        │
        └── TP4056 Module
             │
             ├── B+/B- ──── LiPo 2000mAh (JST-PH 2.0)
             │
             └── OUT+/OUT- ──┬── 3.3V rail (via XIAO onboard LDO)
                             │
                             └── Voltage divider → GPIO 10 (battery ADC)
                                  R1=100kΩ ─┬── VBAT
                                            ├── → GPIO 10
                                  R2=100kΩ ─┴── GND
                                  (divides 4.2V → 2.1V, safe for 3.3V ADC)
```

### Power Management

- **Deep Sleep:** ESP32S3 draws ~10μA in deep sleep
- **Wake Source:** GPIO 8 (ext0 wakeup) — dedicated power button
- **Backlight Control:** GPIO 5 drives ILI9341 BL (digital HIGH/LOW)

---

## BOM Summary

| Component | Qty | Notes |
|-----------|-----|-------|
| XIAO ESP32S3 | 1 | 8MB PSRAM variant |
| ILI9341 2.4" TFT | 1 | 240×320, SPI, no touch |
| M5Stack CardKB | 1 | I2C QWERTY keyboard |
| TP4056 module | 1 | USB-C variant preferred |
| LiPo battery | 1 | 2000mAh, JST-PH 2.0 |
| 2N7000 MOSFET | 1 | Backlight control (optional) |
| 4.7kΩ resistors | 2 | I2C pull-ups |
| 100kΩ resistors | 2 | Battery voltage divider |
| 2.2kΩ resistor | 1 | TP4056 PROG |
| 10μF capacitors | 2 | LDO input/output |
