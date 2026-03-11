// calc_screen.h — Main calculator screen
#pragma once
#include "screen.h"
#include "evaluator.h"
#include <vector>

struct DisplayLine {
    String text;
    bool isExpr;
    bool isError;
};

class CalcScreen : public Screen {
public:
    void draw(Adafruit_ILI9341& tft) override;
    void handleInput(char key) override;
    const char* title() override { return "CALC"; }

private:
    String _input;
    String _lastAnswer;
    std::vector<DisplayLine> _lines;
    Evaluator _eval;
    bool _cursorVisible = true;
    unsigned long _lastBlink = 0;

    void doEvaluate();
    void drawStatusBar(Adafruit_ILI9341& tft);
    void drawMainArea(Adafruit_ILI9341& tft);
    String formatNumber(double v);
};
