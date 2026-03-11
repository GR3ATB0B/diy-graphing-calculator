// calc_screen.cpp — Calculator screen (direct drawing on ILI9341)
#include "calc_screen.h"
#include "config.h"

static const int LINE_H = 16;
static const int MARGIN = 6;

void CalcScreen::handleInput(char key) {
    switch (key) {
        case 0x08:  // Backspace
            if (_input.length() > 0)
                _input.remove(_input.length() - 1);
            break;
        case 0x0D:  // Enter
            doEvaluate();
            break;
        case 0x1B:  // Escape — clear
            if (_input.length() > 0) _input = "";
            else _lines.clear();
            break;
        default:
            if (key >= 0x20 && key <= 0x7E && _input.length() < 60)
                _input += key;
            break;
    }
}

void CalcScreen::doEvaluate() {
    if (_input.length() == 0) return;

    DisplayLine exprLine;
    exprLine.text = _input;
    exprLine.isExpr = true;
    exprLine.isError = false;
    _lines.push_back(exprLine);

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

    while (_lines.size() > 200)
        _lines.erase(_lines.begin());
}

String CalcScreen::formatNumber(double v) {
    char buf[32];
    if (fabs(v) > 1e12 || (fabs(v) < 1e-6 && v != 0)) {
        snprintf(buf, sizeof(buf), "%.8e", v);
    } else {
        snprintf(buf, sizeof(buf), "%.10f", v);
        char* dot = strchr(buf, '.');
        if (dot) {
            char* end = buf + strlen(buf) - 1;
            while (end > dot && *end == '0') *end-- = '\0';
            if (end == dot) *end = '\0';
        }
    }
    return String(buf);
}

void CalcScreen::draw(Adafruit_ILI9341& tft) {
    unsigned long now = millis();
    if (now - _lastBlink > 530) {
        _cursorVisible = !_cursorVisible;
        _lastBlink = now;
    }

    tft.fillScreen(COL_BG);
    drawStatusBar(tft);
    drawMainArea(tft);
}

void CalcScreen::drawStatusBar(Adafruit_ILI9341& tft) {
    tft.fillRect(0, 0, SCREEN_W, STATUS_BAR_H, COL_STATUS_BG);
    tft.setTextColor(COL_DIM);
    tft.setTextSize(1);
    tft.setCursor(MARGIN, 6);
    tft.print("CALC");
}

void CalcScreen::drawMainArea(Adafruit_ILI9341& tft) {
    int areaTop = STATUS_BAR_H + 2;
    int areaBottom = SCREEN_H - 4;

    // Calculate start position (bottom-align)
    int totalLines = _lines.size() + 1;
    int y = areaBottom - totalLines * LINE_H;
    if (y < areaTop) y = areaTop;

    tft.setTextSize(1);

    // History
    for (size_t i = 0; i < _lines.size(); i++) {
        if (y + LINE_H < areaTop) { y += LINE_H; continue; }
        if (y > areaBottom) break;

        if (_lines[i].isExpr)
            tft.setTextColor(COL_TEXT);
        else if (_lines[i].isError)
            tft.setTextColor(COL_ERROR);
        else
            tft.setTextColor(COL_ACCENT);

        // Right-align: calc pixel width (6px per char at size 1)
        int tw = _lines[i].text.length() * 6;
        tft.setCursor(SCREEN_W - MARGIN - tw, y);
        tft.print(_lines[i].text);
        y += LINE_H;
    }

    // Current input
    if (y <= areaBottom) {
        tft.setTextColor(COL_TEXT);
        int tw = _input.length() * 6;
        int cx = SCREEN_W - MARGIN - tw;
        tft.setCursor(cx, y);
        tft.print(_input);

        // Cursor
        if (_cursorVisible) {
            int cursorX = SCREEN_W - MARGIN;
            tft.fillRect(cursorX + 1, y, 2, LINE_H - 2, COL_ACCENT);
        }
    }
}
