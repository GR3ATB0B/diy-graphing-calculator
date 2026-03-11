// calc_screen.cpp — TI-84 Plus CE style calculator screen
// Input via CardKB: full ASCII keyboard, backspace=0x08, enter=0x0D, esc=0x1B
#include "calc_screen.h"

static const int FKEY_H = 16;
static const int LINE_H = 14;
static const int MARGIN_R = 6;

void CalcScreen::handleInput(char key) {
    switch (key) {
        case 0x08:  // Backspace
            if (_input.length() > 0) {
                _input.remove(_input.length() - 1);
            }
            break;

        case 0x0D:  // Enter — evaluate
            doEvaluate();
            break;

        case 0x1B:  // Escape — clear
            if (_input.length() > 0) {
                _input = "";
            } else {
                _lines.clear();
            }
            break;

        default:
            // Accept printable ASCII characters
            if (key >= 0x20 && key <= 0x7E) {
                if (_input.length() < 60) {
                    _input += key;
                }
            }
            break;
    }
}

void CalcScreen::doEvaluate() {
    if (_input.length() == 0) return;

    // Add expression line
    DisplayLine exprLine;
    exprLine.text = _input;
    exprLine.isExpr = true;
    exprLine.isError = false;
    _lines.push_back(exprLine);

    // Evaluate
    EvalResult r = _eval.evaluate(_input);
    DisplayLine resultLine;
    resultLine.isExpr = false;

    if (r.ok) {
        resultLine.text = formatNumber(r.value);
        resultLine.isError = false;
        _lastAnswer = resultLine.text;
    } else {
        resultLine.text = r.error;
        resultLine.isError = true;
    }
    _lines.push_back(resultLine);

    _input = "";

    // Keep history manageable
    while (_lines.size() > 200) {
        _lines.erase(_lines.begin());
    }
}

String CalcScreen::formatNumber(double v) {
    char buf[32];
    if (fabs(v) > 1e12 || (fabs(v) < 1e-6 && v != 0)) {
        snprintf(buf, sizeof(buf), "%.8e", v);
    } else {
        snprintf(buf, sizeof(buf), "%.10f", v);
        // Strip trailing zeros
        char* dot = strchr(buf, '.');
        if (dot) {
            char* end = buf + strlen(buf) - 1;
            while (end > dot && *end == '0') *end-- = '\0';
            if (end == dot) *end = '\0';
        }
    }
    return String(buf);
}

void CalcScreen::draw(TFT_eSprite& fb) {
    // Update cursor blink
    unsigned long now = millis();
    if (now - _lastBlink > 530) {
        _cursorVisible = !_cursorVisible;
        _lastBlink = now;
    }

    fb.fillSprite(COL_BG);
    drawStatusBar(fb);
    drawMainArea(fb);
    drawFkeyLabels(fb);
}

void CalcScreen::drawStatusBar(TFT_eSprite& fb) {
    fb.fillRect(0, 0, SCREEN_W, STATUS_BAR_H, COL_STATUS_BG);

    // Battery icon
    int bx = SCREEN_W - 28, by = 5;
    fb.drawRect(bx, by, 18, 10, COL_DIM);
    fb.fillRect(bx + 18, by + 3, 2, 4, COL_DIM);
    fb.fillRect(bx + 2, by + 2, 12, 6, COL_ACCENT);

    fb.setTextColor(COL_DIM);
    fb.setTextSize(1);
    fb.setTextDatum(ML_DATUM);
    fb.drawString("CALC", 6, STATUS_BAR_H / 2);
}

void CalcScreen::drawMainArea(TFT_eSprite& fb) {
    int areaTop = STATUS_BAR_H + 2;
    int areaBottom = SCREEN_H - FKEY_H - 2;

    fb.setTextSize(1);

    // Count total lines (history + current input)
    int totalLines = _lines.size() + 1; // +1 for current input
    int startY = areaBottom - totalLines * LINE_H;
    if (startY < areaTop) startY = areaTop;

    int y = startY;

    // Draw history lines
    for (size_t i = 0; i < _lines.size(); i++) {
        if (y + LINE_H < areaTop) { y += LINE_H; continue; }
        if (y > areaBottom) break;

        if (_lines[i].isExpr) {
            fb.setTextColor(COL_TEXT);
        } else if (_lines[i].isError) {
            fb.setTextColor(COL_ERROR);
        } else {
            fb.setTextColor(COL_ACCENT);
        }
        fb.setTextDatum(TR_DATUM);
        fb.drawString(_lines[i].text, SCREEN_W - MARGIN_R, y);
        y += LINE_H;
    }

    // Draw current input line
    if (y <= areaBottom) {
        fb.setTextColor(COL_TEXT);
        fb.setTextDatum(TR_DATUM);
        if (_input.length() > 0) {
            fb.drawString(_input, SCREEN_W - MARGIN_R, y);
        }

        // Blinking cursor
        if (_cursorVisible) {
            int textW = fb.textWidth(_input);
            int cx = SCREEN_W - MARGIN_R + 1;
            fb.fillRect(cx, y, 2, LINE_H - 2, COL_ACCENT);
        }
    }
}

void CalcScreen::drawFkeyLabels(TFT_eSprite& fb) {
    int y = SCREEN_H - FKEY_H;
    fb.fillRect(0, y, SCREEN_W, FKEY_H, 0x0421);

    fb.drawLine(0, y, SCREEN_W, y, 0x0841);

    const char* labels[] = {"MATH", "PLOT", "TABLE", "PROG", "VARS"};
    int w = SCREEN_W / 5;

    fb.setTextColor(COL_DIM);
    fb.setTextSize(1);
    fb.setTextDatum(MC_DATUM);
    for (int i = 0; i < 5; i++) {
        fb.drawString(labels[i], w * i + w / 2, y + FKEY_H / 2);
    }
}
