// calc_screen.h — Main calculator screen (TI-84 Plus CE style)
#pragma once
#include "screen.h"
#include "evaluator.h"
#include <vector>

// A line on the screen: either an expression or a result
struct DisplayLine {
    String text;
    bool   isExpr;    // true = user expression, false = result
    bool   isError;   // only relevant when !isExpr
};

class CalcScreen : public Screen {
public:
    void draw(TFT_eSprite& fb) override;
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
    void drawStatusBar(TFT_eSprite& fb);
    void drawMainArea(TFT_eSprite& fb);
    void drawFkeyLabels(TFT_eSprite& fb);
    String formatNumber(double v);
};
