# DIY Graphing Calculator — Design Spec

> Nash's vision doc — March 7, 2026
> Target size: TI-89 form factor (~170 x 80 x 23mm)

---

## UI / Software

### Home Screen
- Boot straight into calculator mode (like TI-84 Plus CE) — no splash screen, no menu, just ready to calculate
- Function keys (F1-F5) mapped to top row, context-sensitive like TI-89

### Navigation
- **Apps button** — opens an apps menu (graphing, tables, programs, settings, etc.)
- **Window switcher** — TI-89 style, toggle between your last two screens or pick from open apps. This is a KEY feature Nash loves.
- **Math menu** — abs, floor, ceiling, nPr, nCr, etc. live inside menus, not on physical keys

### Display
- 2.4" ST7789 IPS, 320x240, no touch
- TI-84 CE style layout: status bar at top, main calculation area, soft-key labels at bottom for F-keys

---

## Keyboard / Input

### Main Layout (TI-89 size footprint)
Top to bottom:
1. **F-key row** — F1 through F5, mapped to on-screen function labels
2. **Modifier row** — Color-coded modifier keys (like TI-89's green/yellow system, NOT labeled "shift" — just colored buttons that activate the color-coded secondary functions)
3. **Navigation row** — Apps, Menu, Settings, Alpha
4. **Science row** — ln, log, e, pi, sin, cos, tan, x^2, sqrt
5. **Number pad** — Big chunky number keys (0-9), decimal point
6. **Operations** — Add, subtract, multiply, divide on the RIGHT side, big and easy to hit
7. **Enter, DEL, Clear** — Bottom area

### QWERTY Add-on (M5Stack CardKB style)
- Nash has an M5Stack CardKB (mini QWERTY keyboard)
- Want this as an OPTIONAL attachment for coding/text input
- Connection options (in order of preference):
  - Magnetic pogo pin connector (snap on/off)
  - Bluetooth LE
  - I2C ribbon cable
- When attached, unlocks full A-Z QWERTY input for coding/text
- When detached, calculator only has X, Y, Z keys (all you need for math/graphing)
- NO alphabetical keyboard on the base unit — just X, Y, Z
- This solves the "alphabetical keyboard sucks for coding" problem while keeping the base unit clean and minimal

### Color-Coded Modifiers
- Inspired by TI-89's green (diamond) and yellow (2nd) keys
- Two modifier buttons, each a distinct color
- Secondary functions printed on keys in matching colors
- More intuitive than generic "shift" / "2nd" labels

---

## Physical Design

### Size
- TI-89 form factor: ~170 x 80 x 23mm — NON-NEGOTIABLE
- Must feel pocketable, not bulky
- The QWERTY keyboard add-on can extend the length when attached but the base unit stays TI-89 sized

### Key Feel
- Tactile switches with 3D-printed keycaps
- Number keys should be LARGE — easy to hit without looking
- Operation keys (+-*/) on the right, also large

---

## Hardware
- MCU: Seeed XIAO ESP32S3 (8MB PSRAM)
- Display: 2.4" ST7789 IPS 320x240 (no touch)
- Battery: ~2000mAh LiPo
- CAS: Eigenmath
- Optional: M5Stack CardKB or similar mini QWERTY via I2C/BLE

---

## Inspiration
- TI-84 Plus CE: boot-to-calculator UX, apps menu, clean home screen
- TI-89: window switcher, color-coded modifiers (green/diamond), F-keys, form factor
- M5Stack CardKB: snap-on QWERTY for when you need real text input
