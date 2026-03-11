// evaluator.cpp — Recursive descent expression parser
// Grammar:
//   expr   = term (('+' | '-') term)*
//   term   = factor (('*' | '/') factor)*
//   factor = ['-'] (number | '(' expr ')')
//   number = digit+ ['.' digit+]

#include "evaluator.h"
#include <math.h>

EvalResult Evaluator::evaluate(const String& expr) {
    _p = expr.c_str();
    EvalResult r;
    try {
        r.value = parseExpr();
        skipSpaces();
        if (*_p != '\0') {
            r.ok = false;
            r.error = "Unexpected: '";
            r.error += *_p;
            r.error += "'";
        } else if (isnan(r.value) || isinf(r.value)) {
            r.ok = false;
            r.error = "Math error";
        } else {
            r.ok = true;
        }
    } catch (...) {
        r.ok = false;
        r.error = "Syntax error";
    }
    return r;
}

void Evaluator::skipSpaces() {
    while (*_p == ' ') _p++;
}

double Evaluator::parseExpr() {
    double val = parseTerm();
    skipSpaces();
    while (*_p == '+' || *_p == '-') {
        char op = *_p++;
        double right = parseTerm();
        if (op == '+') val += right;
        else           val -= right;
        skipSpaces();
    }
    return val;
}

double Evaluator::parseTerm() {
    double val = parseFactor();
    skipSpaces();
    while (*_p == '*' || *_p == '/') {
        char op = *_p++;
        double right = parseFactor();
        if (op == '*') val *= right;
        else           val /= right;
        skipSpaces();
    }
    return val;
}

double Evaluator::parseFactor() {
    skipSpaces();

    // Unary minus
    bool neg = false;
    if (*_p == '-') {
        neg = true;
        _p++;
    }

    double val;

    if (*_p == '(') {
        _p++;  // skip '('
        val = parseExpr();
        skipSpaces();
        if (*_p == ')') _p++;  // skip ')'
    } else {
        val = parseNumber();
    }

    return neg ? -val : val;
}

double Evaluator::parseNumber() {
    skipSpaces();
    const char* start = _p;

    while (*_p >= '0' && *_p <= '9') _p++;
    if (*_p == '.') {
        _p++;
        while (*_p >= '0' && *_p <= '9') _p++;
    }

    if (_p == start) {
        // No number found — return NaN to signal error
        return NAN;
    }

    // Convert substring to double
    char buf[32];
    int len = _p - start;
    if (len > 30) len = 30;
    memcpy(buf, start, len);
    buf[len] = '\0';
    return atof(buf);
}
