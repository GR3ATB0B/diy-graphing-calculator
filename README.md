# DIY Graphing Calculator

An open-source graphing calculator built around the ESP32-S3, targeting a TI-89 form factor with computer algebra system (CAS) capabilities.

## Project Goals

- Build a fully functional graphing calculator from scratch
- Symbolic math (derivatives, integrals, simplification) via Eigenmath CAS
- Color graphing with function plotting, zoom, and pan
- Pretty-print math rendering (fractions, exponents, radicals)
- Rechargeable LiPo battery with 7-13 hours of use
- 3D-printed enclosure in the TI-89 form factor (~170 × 80 × 23mm)
- WiFi-enabled for firmware updates and data transfer

## Planned Specs

| Spec | Detail |
|------|--------|
| MCU | ESP32-S3 (dual-core LX7 @ 240MHz, 8MB PSRAM, WiFi + BLE) |
| Display | 2.8" ILI9341 320×240 TFT (IPS, resistive touch) |
| CAS Engine | Eigenmath (C) with MicroPython bindings |
| Firmware | MicroPython (UI/app layer) + C modules (CAS, graphics) |
| Input | Tactile switch matrix (prototype), silicone rubber dome (final) |
| Battery | 3.7V 2000mAh LiPo with TP4056 USB-C charging |
| Enclosure | 3D-printed PLA/PETG, TI-89 form factor |

## Bill of Materials

| Component | Part | Est. Price |
|-----------|------|-----------|
| MCU | Seeed XIAO ESP32S3 (8MB PSRAM) | $8 |
| Display | 2.8" ILI9341 320×240 TFT w/ touch | $7 |
| Buttons | 4×5 membrane keypad + tactile switches | $5 |
| Battery | 3.7V 2000mAh LiPo (804060) | $6 |
| Charger | TP4056 USB-C module w/ protection | $1 |
| Wiring | Dupont jumpers, breadboard | $5 |
| Filament | PLA/PETG for enclosure | ~$2 |
| **Total** | | **~$34** |

## Build Phases

1. **Phase 1 — Display + MCU:** Get ESP32-S3 driving the 2.8" TFT. Flash MicroPython, render text and basic graphics.
2. **Phase 2 — Input:** Wire up membrane keypad, build a basic 4-function calculator with on-screen display.
3. **Phase 3 — CAS Integration:** Port Eigenmath to ESP32-S3 (following Galdeano project's approach). Symbolic differentiation and integration.
4. **Phase 4 — Graphing Engine:** Function plotting with axes, labels, zoom/pan, multiple traces.
5. **Phase 5 — Math Rendering:** Pretty-print renderer for stacked fractions, exponents, radicals, integral signs.
6. **Phase 6 — Enclosure:** Design and 3D-print the case. Final button layout with tactile switches or silicone dome pad.
7. **Phase 7 — Polish:** Equation solver, matrix operations, UI refinements, power management.

## Inspiration Projects

| Project | Description | Link |
|---------|-------------|------|
| Galdeano Handheld | ESP32 + Eigenmath CAS calculator (primary reference) | [Hackaday.io](https://hackaday.io/project/187213-galdeano-handheld-computer) |
| sci-calc | Open-source ESP32 scientific calculator with custom PCB | [GitHub](https://github.com/shaoxiongduan/sci-calc) |
| ESP32-Calculator | ESP32-based graphing calculator | [GitHub](https://github.com/ChromeUniverse/ESP32-Calculator) |
| NumWorks Epsilon | Fully open-source calculator firmware (excellent math renderer) | [GitHub](https://github.com/numworks/epsilon) |
| Eigenmath | Lightweight symbolic math engine in C | [eigenmath.org](http://eigenmath.org/) |
| TFT_eSPI | Gold-standard ESP32 TFT display library | [GitHub](https://github.com/Bodmer/TFT_eSPI) |

## Repository Structure

```
├── docs/           # Research notes, build log
├── firmware/       # ESP32 code (MicroPython + C modules)
├── hardware/       # KiCad schematics, PCB layouts, 3D print STLs
└── README.md
```

## License

MIT — see [LICENSE](LICENSE)
