# Nash Calculator — Web Emulator

Visual emulator matching the ESP32 firmware's display pixel-for-pixel.

## Usage

Just open `index.html` in any browser. No build step, no dependencies.

```bash
open index.html
# or
python3 -m http.server 8080  # then visit localhost:8080
```

## Controls

**On-screen keypad** — click the buttons

**Keyboard shortcuts:**
- `0-9` — digits
- `+ - * /` — operators
- `( )` — parentheses
- `.` — decimal point
- `Enter` — evaluate
- `Backspace` — delete last character

## What It Emulates

- 320×240 display (scaled 2× for visibility)
- Status bar with battery icon + title
- Expression history (scrolls up)
- Input line at bottom
- Same recursive descent evaluator as firmware (proper PEMDAS)
- DM42n-inspired dark minimal aesthetic
