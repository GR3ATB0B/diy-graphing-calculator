# Nash Calculator — Button Layout

> TI-89 form factor (~170 × 80 mm) · 48 keys · Color-coded modifiers

---

## ASCII Layout

```
┌──────────────────────────────────────────────────────┐
│  ┌──────────────── 2.4" Display ───────────────────┐ │
│  │  STATUS: CALC              ▔▔▔▔ 98%             │ │
│  │                                                  │ │
│  │                          3+5                     │ │
│  │                            8                     │ │
│  │                        12/4                      │ │
│  │                            3                     │ │
│  │                                                  │ │
│  │  F1     F2     F3     F4     F5    ← soft labels │ │
│  └──────────────────────────────────────────────────┘ │
│                                                        │
│  [F1]   [F2]   [F3]   [F4]   [F5]        ← Row 0     │
│                                                        │
│  [2ND]  [ALPHA] [Apps] [Menu] [Settings]  ← Row 1     │
│   ●ylw   ●grn                                         │
│                                                        │
│  [ ln ] [log ] [ e  ] [ π  ] [x,y,z]     ← Row 2     │
│                                                        │
│  [sin ] [cos ] [tan ] [ x² ] [ √  ]      ← Row 3     │
│                                                        │
│  [ ( ] [ ) ] [EXP] [ANS] [ DEL ]         ← Row 4     │
│                                                        │
│                  [▲]                                   │
│  [ X ] [ Y ] [◄][OK][►] [ Z ]            ← Row 5     │
│                  [▼]                                   │
│                                                        │
│  [ 7 ] [ 8 ] [ 9 ]  [ ÷ ]               ← Row 6     │
│  [ 4 ] [ 5 ] [ 6 ]  [ × ]               ← Row 7     │
│  [ 1 ] [ 2 ] [ 3 ]  [ − ]               ← Row 8     │
│  [ 0 ] [ . ] [(−)] [ + ]                ← Row 9     │
│                                                        │
│       [CLEAR]        [ENTER]              ← Row 10    │
│                                                        │
└──────────────────────────────────────────────────────┘
```

---

## Key Map (48 keys)

| Row | Col 0 | Col 1 | Col 2 | Col 3 | Col 4 |
|-----|-------|-------|-------|-------|-------|
| 0   | F1    | F2    | F3    | F4    | F5    |
| 1   | 2ND   | ALPHA | Apps  | Menu  | Settings |
| 2   | ln    | log   | e     | π     | x,y,z |
| 3   | sin   | cos   | tan   | x²    | √     |
| 4   | (     | )     | EXP   | ANS   | DEL   |
| 5   | X / ◄ | Y / ▲▼ | Z / ► | ▲   | ▼     |
| 6   | 7     | 8     | 9     | ÷     |       |
| 7   | 4     | 5     | 6     | ×     |       |
| 8   | 1     | 2     | 3     | −     |       |
| 9   | 0     | .     | (−)   | +     |       |
| 10  | CLEAR (wide) |  | ENTER (wide) |  |   |

---

## Color-Coded Modifier System

| Modifier | Key Color | Print Color | Example |
|----------|-----------|-------------|---------|
| Normal   | —         | White text  | sin     |
| **2ND**  | Yellow    | Yellow text | sin⁻¹   |
| **ALPHA**| Green     | Green text  | A       |

### 2ND Functions (Yellow)

| Key   | 2ND Function |
|-------|-------------|
| sin   | sin⁻¹       |
| cos   | cos⁻¹       |
| tan   | tan⁻¹       |
| ln    | eˣ          |
| log   | 10ˣ         |
| x²    | x³          |
| √     | ³√          |
| (     | [           |
| )     | ]           |
| EXP   | ×10ⁿ        |
| ANS   | ENTRY (recall last expr) |
| DEL   | INS         |
| CLEAR | RESET       |
| F1-F5 | Context menus |

### ALPHA Functions (Green)

Number keys map to letters when ALPHA is held:
- 0→Space, 1→A, 2→B, 3→C, 4→D, 5→E, 6→F, 7→G, 8→H, 9→I
- Additional letters via repeated ALPHA presses (like phone T9)

---

## Physical Key Sizes

- **Number keys (0-9):** 12×12mm — large, tactile
- **Operator keys (+−×÷):** 12×12mm — same size, right column
- **ENTER:** 24×12mm — double-wide
- **CLEAR:** 24×12mm — double-wide
- **F1-F5:** 10×8mm — smaller, top row
- **Arrow keys:** 8×8mm — compact cluster
- **Science/function keys:** 10×10mm — medium

---

## Notes

- Arrow key cluster uses a D-pad style layout with OK/select in center
- X, Y, Z are separate physical keys (no ALPHA needed for variables)
- (−) is a dedicated negative/sign key, separate from minus operator
- QWERTY add-on (M5Stack CardKB) connects via I2C for full text input
