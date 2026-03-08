# DIY Python Graphing Calculator — Hardware & Software Research

> Target: ESP32-S3 based, TI-89 form factor (~170×80×23mm), MicroPython/C, CAS + graphing + pretty-print math
> Updated: March 2026

---

## 1. Brain — ESP32-S3 Dev Boards

All boards below: dual-core Xtensa LX7 @ 240MHz, WiFi + BLE 5.0, native USB.

| Board | Flash | PSRAM | Size (mm) | Price | Notes |
|-------|-------|-------|-----------|-------|-------|
| **Seeed XIAO ESP32S3** | 8MB | 8MB | 21×17.5 | ~$8 | ⭐ **Best pick.** Tiny, 8MB PSRAM, battery charge pin, castellated pads. No camera version is cheaper. |
| **Adafruit QT Py ESP32-S3** | 4MB | 2MB | 21.7×17.8 | ~$10 | STEMMA QT connector, great docs. Only 2MB PSRAM — may be tight for CAS. |
| **Waveshare ESP32-S3-Mini** | 8-16MB | 8MB | ~25×18 | ~$7 | Good value, multiple flash/PSRAM configs on AliExpress. |
| **ESP32-S3-DevKitC-1 (Espressif)** | 8-16MB | 8MB | 69×25 | ~$8 | Official reference board. Too big for final build but great for prototyping. |
| **LilyGo T7-S3** | 16MB | 8MB | ~50×25 | ~$9 | Has battery connector + charging IC built in. Good for prototyping with battery. |

**Recommendation:** Start prototyping with **XIAO ESP32S3** (8MB PSRAM is critical for CAS). For the final build, consider the bare **ESP32-S3-WROOM-1-N16R8 module** (~$3-4) soldered onto a custom PCB.

---

## 2. Display — Color TFT/IPS Screens

For math notation rendering, you want **at least 320×240**. Higher is better but SPI bandwidth limits refresh rate.

| Display | Size | Resolution | Driver | Touch | Price | Notes |
|---------|------|------------|--------|-------|-------|-------|
| **ILI9341 TFT (generic)** | 2.8" | 320×240 | ILI9341 | Resistive (optional) | $5-8 | ⭐ **Best starting point.** Tons of ESP32 tutorials, TFT_eSPI support is excellent. |
| **ST7789 IPS** | 2.4" | 320×240 | ST7789 | No | $4-6 | Better viewing angles (IPS), no touch. Great colors. |
| **ILI9341 IPS w/ touch** | 2.8" | 320×240 | ILI9341 | Resistive | $7-10 | IPS variant with touch. Best balance. |
| **ST7789 IPS** | 3.2" | 240×320 | ST7789V2 | Cap touch optional | $8-12 | Bigger screen for graphs, but still 240×320. |
| **ILI9488** | 3.5" | 480×320 | ILI9488 | Resistive/Cap | $10-15 | Higher res but SPI is slow at this size. Consider parallel interface. |

**Recommendation:** Go with a **2.8" ILI9341 320×240 with resistive touch** — ubiquitous, cheap, great library support. TFT_eSPI library handles it perfectly on ESP32-S3. If you don't need touch (physical buttons only), the **2.4" ST7789 IPS** has better colors.

**Key library:** [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) — the gold standard for ESP32 TFT driving. Supports all these controllers, SPI DMA, sprite buffering.

---

## 3. Input / Buttons

### Option A: Membrane Keypad Matrix (Easiest Start)
- **4×5 or 4×4 matrix keypads** — $2-4 on Amazon/AliExpress
- Flat, thin, fits in enclosure easily
- Feels mushy — not great for daily use
- Can layer custom printed overlays for calculator labels

### Option B: Tactile Switch Matrix (Best DIY Balance)
- Buy **6×6mm tactile switches** in bulk (~$3 for 100)
- Wire as matrix (e.g., 5×8 = 40 keys, uses only 13 GPIO pins)
- 3D-print keycaps that snap onto the switches
- Feels clicky and responsive
- **TI-89 has ~50 keys** — a 6×9 matrix (15 GPIO) covers it

### Option C: Silicone Rubber Dome Buttons (Pro Feel) ⭐
- This is what real calculators use
- **How to DIY:** Design a custom PCB with exposed copper pads, then get a **silicone rubber keypad** made
- **PCBWay/JLCPCB** can make custom silicone keypads for ~$30-50 (low MOQ)
- Carbon pill contacts under silicone domes — tactile, quiet, great feel
- For prototyping: buy generic **silicone rubber button pads** from AliExpress (TV remote style) and adapt

### Option D: Cheap CYD / Touchscreen Only
- Skip physical buttons entirely, use on-screen keyboard
- The "Cheap Yellow Display" (ESP32-2432S028R) is a ~$10 all-in-one with 2.8" touch + ESP32
- Not great for a "real calculator" feel but simplifies hardware massively

### Recommended Approach:
1. **Prototype** with a 4×5 membrane keypad + a few extra tactile buttons for special keys
2. **Final build** with a custom PCB + silicone rubber dome pad (or a tactile switch matrix with 3D-printed keycaps)

### Scanning a Matrix with ESP32-S3:
- Use GPIO with internal pull-ups, scan columns, read rows
- MicroPython: simple polling loop, or use interrupts
- Libraries: [keypad module in CircuitPython](https://docs.circuitpython.org/en/latest/shared-bindings/keypad/index.html), or roll your own in MicroPython (it's ~30 lines)

---

## 4. Battery & Power

### LiPo Battery Options
| Battery | Capacity | Size (mm) | Price | Notes |
|---------|----------|-----------|-------|-------|
| 103450 LiPo | 1800mAh | 51×34×10 | ~$5 | Fits well in TI-89 enclosure |
| 804060 LiPo | 2000mAh | 60×40×8 | ~$6 | Flat, good for behind the display |
| 603048 LiPo | 900mAh | 48×30×6 | ~$4 | Smaller option if space is tight |

### Charging Circuit
- **TP4056 module (with protection)** — $0.50-1, handles LiPo charging via USB-C/micro-USB
  - Get the version with DW01A protection IC (overdischarge/overcharge protection)
  - LED indicators for charge status
- **Better option:** Many ESP32-S3 boards (LilyGo T7-S3, some XIAO variants) have **built-in LiPo charging** via the USB-C port. Just plug in a battery.
- **Boost converter** if needed: MT3608 or similar, $0.50, to get stable 5V for display backlight

### Power Budget Estimate
- ESP32-S3 active: ~100-200mA
- TFT backlight: ~40-80mA
- **Total: ~150-280mA** → a 2000mAh battery gives **7-13 hours** of use
- Deep sleep: ~10-20μA (excellent standby)

---

## 5. Math / CAS Engine

This is the hardest part of the project. Here are your options, ranked by practicality:

### ⭐ Option 1: Eigenmath (C, proven on ESP32)
- **What:** Lightweight symbolic math engine written in C
- **Capabilities:** Derivatives, integrals, simplification, matrices, plotting expressions
- **ESP32 proven:** The **Galdeano project** (see Section 8) successfully runs Eigenmath on ESP32 with MicroPython wrapper
- **Source:** [eigenmath.org](http://eigenmath.org/) — original by George Weigt
- **Casio port:** [github.com/gbl08ma/eigenmath](https://github.com/gbl08ma/eigenmath) — already adapted for constrained hardware
- **Memory:** Fits in ~200KB RAM with dynamic allocation tweaks
- **This is your best bet.** Proven, C-based, already ported to embedded calculators.

### Option 2: Mathomatic (C)
- Lightweight CAS in pure C, ~20K lines of code
- Does: simplification, solving, derivatives, Laplace transforms
- **No integration** (major limitation)
- Good for equation solving, less for calculus
- [mathomatic.org](http://www.mathomatic.org/) (archived, but source available)

### Option 3: MicroPython `eval()` + Custom Math
- Use Python's built-in `math` module for numerical computation
- Write your own symbolic differentiation using expression trees (it's a great CS project!)
- Numerical integration via Simpson's rule / trapezoidal rule
- Limited but educational — you learn a TON

### Option 4: tinyexpr (C, expression parsing only)
- [github.com/codeplea/tinyexpr](https://github.com/codeplea/tinyexpr)
- Only does **numerical** expression evaluation — no symbolic math
- Very small footprint, good for the graphing/evaluation layer
- Could pair with Eigenmath: Eigenmath for CAS, tinyexpr for fast numerical eval during graphing

### Option 5: Port a subset of SymPy
- SymPy is massive (100K+ lines), won't fit on ESP32 as-is
- **uSymPy** doesn't exist as a maintained project
- You could port a tiny subset (differentiation rules, basic simplification) — big project but doable
- Better to use Eigenmath

### Recommendation:
**Use Eigenmath** as the CAS core (C, compiled with ESP-IDF), wrap it with MicroPython bindings for the UI layer. Use **tinyexpr** or direct float evaluation for fast graphing passes.

---

## 6. Pretty-Print / Natural Display Math Rendering

This is what makes it look like a real graphing calculator — stacked fractions, proper exponents, integral signs, etc.

### Approach 1: Custom 2D Layout Engine (Most Common)
Build a simple recursive renderer:
- Parse expression into a tree
- Each node knows its width/height
- Fractions: numerator above denominator with a line
- Exponents: smaller font, raised position
- Radicals: draw √ symbol + overline
- Integrals: draw ∫ symbol with limits

**This is how TI and Casio do it.** It's not as hard as it sounds — the core layout algorithm is ~200-400 lines of code.

### Approach 2: Bitmap Font + Sprites
- Create bitmap sprites for math symbols (∫, √, Σ, π, etc.)
- Use TFT_eSPI's sprite system for compositing
- Multiple font sizes for exponents/subscripts (e.g., 16px normal, 10px super/subscript)
- Adafruit GFX compatible fonts work well

### Approach 3: LaTeX-style Rendering
- Implement a tiny subset of TeX layout rules
- Overkill for ESP32 but produces beautiful output
- No existing ESP32 library for this — you'd be building from scratch

### Practical Steps:
1. **Get good fonts:** Use [u8g2](https://github.com/olikraus/u8g2) fonts or convert TTF fonts with Adafruit's font converter
2. **Define a math AST** (Abstract Syntax Tree) that your renderer walks
3. **Two-pass rendering:** First pass calculates bounding boxes, second pass draws
4. **Eigenmath outputs text-based expressions** — you need to parse these into your display tree

### Font Resources:
- [TFT_eSPI custom fonts](https://github.com/Bodmer/TFT_eSPI/tree/master/Tools) — includes font converter
- Latin Modern Math or Computer Modern for that classic math textbook look
- For pixel displays: Terminus, Tamzen, or custom bitmap fonts

---

## 7. Graphing / Function Plotting

### Basic Approach (What Everyone Does):
1. Define viewport: x_min, x_max, y_min, y_max
2. For each pixel column x (0 to 319):
   - Map pixel x → math x value
   - Evaluate f(x)
   - Map result → pixel y
   - Draw pixel (or line to previous point)
3. Draw axes, grid, labels

### Making It Good:
- **Anti-discontinuity:** Don't draw lines across asymptotes (check if Δy > threshold)
- **Adaptive sampling:** More points near curves with high curvature
- **Auto-scaling:** Analyze function to find good y-range
- **Multiple functions:** Different colors (TFT_eSPI supports 65K colors)
- **Zoom/pan:** Map touch gestures or button combos to viewport changes

### Libraries:
- **No direct "matplotlib for ESP32" exists** — you'll write the plotting yourself
- **TFT_eSPI** provides: `drawPixel()`, `drawLine()`, `drawFloat()`, `fillRect()` — that's all you need
- For numerical evaluation during plotting, **tinyexpr** is fast and lightweight

### Performance Tips:
- Use **TFT_eSPI sprites** (off-screen buffer) to render the graph, then push to display in one shot — avoids flicker
- ESP32-S3 with PSRAM can buffer a full 320×240×2 frame (150KB) easily
- DMA SPI transfer while computing the next frame

---

## 8. Similar / Inspiration Projects

### ⭐ Galdeano Handheld (THE reference project)
- **What:** ESP32-based graphing calculator with CAS, MicroPython + Eigenmath
- **Creator:** Angel Cabello (otosan-maker)
- **Features:** Symbolic differentiation/integration, graphing, matrix ops, WiFi
- **CAS Engine:** Eigenmath (C) wrapped for MicroPython
- **Display:** Small TFT
- **Buttons:** Custom membrane/tactile matrix
- **Links:**
  - [Hackaday.io project page](https://hackaday.io/project/187213-galdeano-handheld-computer)
  - [Hackster.io writeup](https://www.hackster.io/otosan-maker/galdeano-02-5dd4f4)
  - [Adafruit blog coverage](https://blog.adafruit.com/2023/10/24/a-pocket-micropython-calculator-with-hidden-capabilities-esp32-micropython-calculators-hacksterio/)
- **Key lesson:** They had to switch Eigenmath from static to dynamic memory allocation for ESP32. Follow their patches!

### ChromeUniverse/ESP32-Calculator
- **What:** ESP32-based graphing calculator
- **GitHub:** [github.com/ChromeUniverse/ESP32-Calculator](https://github.com/ChromeUniverse/ESP32-Calculator)
- Simpler than Galdeano but good code reference

### shaoxiongduan/sci-calc
- **What:** Open-source ESP32 scientific calculator with custom PCB
- **GitHub:** [github.com/shaoxiongduan/sci-calc](https://github.com/shaoxiongduan/sci-calc)
- **Features:** Arduino IDE, custom PCB + 3D-printed case, scientific functions
- **Good reference for:** PCB layout, button matrix design, enclosure
- [Circuit Digest writeup](https://circuitdigest.com/news/sci-calc-an-opensource-multifunctional-scientific-calculator-using-esp32)

### ThingPulse Color Kit Grande Calculator
- **What:** Simple calculator demo on ESP32 + color display
- **Link:** [thingpulse.com](https://thingpulse.com/fun-with-the-color-kit-grande-building-a-simple-calculator-with-esp32/)
- Good for touchscreen calculator UI patterns

### baboomerang/Graphing-Calculator-Project
- **What:** TI-84 replacement project (not ESP32-specific but good algorithms)
- **GitHub:** [github.com/baboomerang/Graphing-Calculator-Project](https://github.com/baboomerang/Graphing-Calculator-Project)

### NumWorks Calculator (Open Source!)
- **Not ESP32** but the entire firmware is open source
- Hardware is STM32-based, software is C++
- Their **math rendering engine** is excellent reference code
- **GitHub:** [github.com/numworks/epsilon](https://github.com/numworks/epsilon)
- Study their `poincare` library for expression layout and rendering — best open-source math renderer for calculators

---

## 9. Enclosure — 3D Printable Case

### TI-89 Reference Dimensions
- **Overall:** 170 × 80 × 23mm
- **Screen window:** ~75 × 55mm
- **Battery compartment:** ~60 × 40 × 10mm (4× AAA in original)

### Design Approach
1. **Two-shell design:** Top shell (screen + button cutouts) + bottom shell (battery + PCB)
2. **Snap-fit or M2 screw posts** for assembly
3. **Display bezel** with cutout slightly smaller than display active area
4. **Button wells** in the top shell for key travel (~1-2mm)
5. **USB-C cutout** on side or bottom edge for charging

### Materials
- **PLA:** Fine for prototyping, slightly brittle
- **PETG:** Better for final — slight flex, more durable
- **TPU:** Use for button pad overlay if making flexible keycaps

### Tips
- Print top shell face-down for smooth button surface
- Use **heat-set brass inserts** (M2 or M2.5) for screw posts — way better than threading into plastic
- Leave 0.3-0.5mm tolerance on all snap fits
- Model in **Fusion 360** (free for students) or **OnShape** (free, web-based)

### Existing Enclosure References
- Search Thingiverse/Printables for "handheld calculator case" or "ESP32 handheld"
- The **sci-calc project** (Section 8) has STL files for a calculator enclosure
- [Adafruit PyPortal](https://www.thingiverse.com/search?q=pyportal+case) cases are similar size — good starting point to modify

---

## 10. Bill of Materials — Starter Kit

| Component | Specific Part | Price | Where |
|-----------|--------------|-------|-------|
| MCU | Seeed XIAO ESP32S3 (8MB PSRAM) | $8 | Seeed Studio, Amazon |
| Display | 2.8" ILI9341 320×240 TFT w/ touch | $7 | Amazon, AliExpress |
| Buttons (proto) | 4×5 membrane keypad + 10× tactile switches | $5 | Amazon |
| Battery | 3.7V 2000mAh LiPo (804060) | $6 | Amazon, AliExpress |
| Charger | TP4056 USB-C module w/ protection | $1 | AliExpress (5-pack ~$3) |
| Wiring | Dupont jumper wires, breadboard | $5 | Amazon |
| Filament | PLA/PETG for enclosure | ~$2 | (assuming you have a printer) |
| **Total** | | **~$34** | |

---

## 11. Recommended Build Order

1. **Week 1:** Get XIAO ESP32S3 + 2.8" TFT working. Flash MicroPython, display "Hello World" with TFT_eSPI or MicroPython display driver.
2. **Week 2:** Wire up membrane keypad, build basic 4-function calculator with on-screen display.
3. **Week 3:** Port Eigenmath to ESP32-S3 (study Galdeano project's approach). Get symbolic differentiation working.
4. **Week 4:** Build the graphing engine — plot y=sin(x) with axes and labels.
5. **Week 5:** Pretty-print renderer — stacked fractions, exponents on display.
6. **Week 6:** Design and print enclosure, wire up final button layout.
7. **Week 7-8:** Polish, add features (equation solver, matrix ops, etc.)

---

## Key Links Summary

| Resource | URL |
|----------|-----|
| Galdeano (main inspiration) | https://hackaday.io/project/187213-galdeano-handheld-computer |
| Eigenmath CAS engine | http://eigenmath.org/ |
| TFT_eSPI library | https://github.com/Bodmer/TFT_eSPI |
| sci-calc (ESP32 calculator) | https://github.com/shaoxiongduan/sci-calc |
| ESP32-Calculator | https://github.com/ChromeUniverse/ESP32-Calculator |
| NumWorks (open-source calc firmware) | https://github.com/numworks/epsilon |
| tinyexpr (fast eval) | https://github.com/codeplea/tinyexpr |
| XIAO ESP32S3 | https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html |
