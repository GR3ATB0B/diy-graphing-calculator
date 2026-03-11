// evaluator.h — Expression parser with proper operator precedence
// Supports: +, -, *, /, parentheses, decimals, negative numbers
#pragma once
#include <Arduino.h>

struct EvalResult {
    bool   ok;
    double value;
    String error;
};

class Evaluator {
public:
    EvalResult evaluate(const String& expr);

private:
    const char* _p;

    double parseExpr();
    double parseTerm();
    double parseFactor();
    double parseNumber();
    void   skipSpaces();
};
