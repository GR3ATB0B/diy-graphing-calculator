// ============================================================
//  Nash's DIY Graphing Calculator — Single-File Firmware
//  MCU: Seeed XIAO ESP32S3
//  Display: ILI9341 240×320 SPI (landscape 320×240)
//  Input: M5Stack CardKB (I2C)
//  Hardware SPI @ 40MHz, partial redraws, menu/graph/solver
// ============================================================

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>
#include <vector>

// ============================================================
//  PIN DEFINITIONS
// ============================================================
#define TFT_CS    2
#define TFT_DC    3
#define TFT_RST   4
#define TFT_MOSI  9
#define TFT_CLK   7
#define TFT_BLK   5
#define KB_SDA    1
#define KB_SCL    6
#define KB_ADDR   0x5F

// ============================================================
//  CONSTANTS
// ============================================================
constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 240;
constexpr int STATUS_BAR_H = 20;
constexpr unsigned long CARDKB_POLL_MS = 20;

// Colors (16-bit 565)
constexpr uint16_t COL_BG        = 0x0000;
constexpr uint16_t COL_TEXT      = 0xFFFF;
constexpr uint16_t COL_DIM       = 0x7BEF;
constexpr uint16_t COL_ACCENT    = 0x07FF;
constexpr uint16_t COL_STATUS_BG = 0x0841;
constexpr uint16_t COL_ERROR     = 0xF800;
constexpr uint16_t COL_GRAPH     = 0x07E0;  // Green for graph line
constexpr uint16_t COL_AXES      = 0x4208;  // Dark gray for axes
constexpr uint16_t COL_MENU_SEL  = 0x001F;  // Blue highlight

// Text sizes
constexpr int TEXT_MAIN = 3;   // Main input/output
constexpr int TEXT_HIST = 2;   // History lines
constexpr int TEXT_STATUS = 1; // Status bar

// Character dimensions per text size (6x8 base)
constexpr int CHAR_W_MAIN = 6 * TEXT_MAIN;  // 18
constexpr int CHAR_H_MAIN = 8 * TEXT_MAIN;  // 24
constexpr int CHAR_W_HIST = 6 * TEXT_HIST;  // 12
constexpr int CHAR_H_HIST = 8 * TEXT_HIST;  // 16
constexpr int LINE_H_MAIN = CHAR_H_MAIN + 4;
constexpr int LINE_H_HIST = CHAR_H_HIST + 2;

static const int MARGIN = 6;

// ============================================================
//  APP MODES
// ============================================================
enum AppMode {
    MODE_CALCULATOR,
    MODE_MENU,
    MODE_GRAPH,
    MODE_GRAPH_INPUT,
    MODE_SOLVER,
    MODE_SOLVER_INPUT
};

AppMode currentMode = MODE_CALCULATOR;
int menuSelection = 0;
constexpr int MENU_ITEMS = 3;
const char* menuLabels[] = { "Calculator", "Graph", "Solver" };

// ============================================================
//  DISPLAY — Hardware SPI @ 40MHz
// ============================================================
Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);

// ============================================================
//  EXPRESSION EVALUATOR — recursive descent parser
//  Supports: + - * / ^ ( ) decimal numbers, unary minus, x variable
// ============================================================
struct EvalResult {
    bool ok;
    double value;
    String error;
};

class Evaluator {
public:
    double xVal = 0;     // Current x value for graphing
    bool useX = false;   // Whether to allow 'x' variable

    EvalResult evaluate(const String& expr) {
        _p = expr.c_str();
        EvalResult r;
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
        return r;
    }

private:
    const char* _p;

    void skipSpaces() { while (*_p == ' ') _p++; }

    double parseExpr() {
        double val = parseTerm();
        skipSpaces();
        while (*_p == '+' || *_p == '-') {
            char op = *_p++;
            double right = parseTerm();
            if (op == '+') val += right; else val -= right;
            skipSpaces();
        }
        return val;
    }

    double parseTerm() {
        double val = parsePower();
        skipSpaces();
        while (*_p == '*' || *_p == '/') {
            char op = *_p++;
            double right = parsePower();
            if (op == '*') val *= right; else val /= right;
            skipSpaces();
        }
        return val;
    }

    double parsePower() {
        double val = parseFactor();
        skipSpaces();
        if (*_p == '^') {
            _p++;
            double right = parsePower(); // right-associative
            val = pow(val, right);
        }
        return val;
    }

    double parseFactor() {
        skipSpaces();
        bool neg = false;
        if (*_p == '-') { neg = true; _p++; }

        double val;
        if (*_p == '(') {
            _p++;
            val = parseExpr();
            skipSpaces();
            if (*_p == ')') _p++;
        } else if (useX && (*_p == 'x' || *_p == 'X')) {
            _p++;
            val = xVal;
        } else {
            val = parseNumber();
        }
        return neg ? -val : val;
    }

    double parseNumber() {
        skipSpaces();
        const char* start = _p;
        while (*_p >= '0' && *_p <= '9') _p++;
        if (*_p == '.') { _p++; while (*_p >= '0' && *_p <= '9') _p++; }
        if (_p == start) return NAN;
        char buf[32];
        int len = _p - start;
        if (len > 30) len = 30;
        memcpy(buf, start, len);
        buf[len] = '\0';
        return atof(buf);
    }
};

// ============================================================
//  CARDKB INPUT
// ============================================================
class CardKB {
public:
    void begin() {
        Wire.begin(KB_SDA, KB_SCL);
        Wire.setClock(100000);
        Wire.beginTransmission(KB_ADDR);
        _connected = (Wire.endTransmission() == 0);
    }

    char read() {
        unsigned long now = millis();
        if (now - _lastPoll < CARDKB_POLL_MS) return '\0';
        _lastPoll = now;
        if (!_connected) return '\0';
        Wire.requestFrom((uint8_t)KB_ADDR, (uint8_t)1);
        if (Wire.available()) {
            char c = Wire.read();
            if (c != 0) return c;
        }
        return '\0';
    }

    bool isConnected() const { return _connected; }

private:
    bool _connected = false;
    unsigned long _lastPoll = 0;
};

// ============================================================
//  CALCULATOR STATE
// ============================================================
struct DisplayLine {
    String text;
    bool isExpr;
    bool isError;
};

String inputBuf;
String lastAnswer;
std::vector<DisplayLine> lines;
Evaluator evaluator;
CardKB cardkb;
bool cursorVisible = true;
unsigned long lastBlink = 0;
bool needsFullRedraw = true;   // Track when full redraw needed
String prevInputBuf;           // Track previous input for partial redraw
size_t prevLineCount = 0;      // Track history changes

// Graph state
String graphExpr;
float graphXmin = -10, graphXmax = 10;
float graphYmin = -10, graphYmax = 10;

// Solver state
int solverStep = 0;        // 0=type select, 1=enter a, 2=enter b, 3=enter c, 4=show result
bool solverQuadratic = false;
double solverA = 0, solverB = 0, solverC = 0;
String solverInput;
String solverResult;

// ============================================================
//  NUMBER FORMATTING
// ============================================================
String formatNumber(double v) {
    char buf[32];
    if (fabs(v) > 1e12 || (fabs(v) < 1e-6 && v != 0)) {
        snprintf(buf, sizeof(buf), "%.6e", v);
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

// ============================================================
//  EVALUATE INPUT EXPRESSION (Calculator mode)
// ============================================================
void doEvaluate() {
    if (inputBuf.length() == 0) return;

    DisplayLine exprLine;
    exprLine.text = inputBuf;
    exprLine.isExpr = true;
    exprLine.isError = false;
    lines.push_back(exprLine);

    evaluator.useX = false;
    EvalResult r = evaluator.evaluate(inputBuf);
    DisplayLine resultLine;
    resultLine.isExpr = false;

    if (r.ok) {
        resultLine.text = formatNumber(r.value);
        resultLine.isError = false;
        lastAnswer = resultLine.text;
    } else {
        resultLine.text = r.error;
        resultLine.isError = true;
    }
    lines.push_back(resultLine);
    inputBuf = "";

    while (lines.size() > 200)
        lines.erase(lines.begin());

    needsFullRedraw = true;
}

// ============================================================
//  SCREEN DRAWING — Partial redraws where possible
// ============================================================

void drawStatusBar() {
    tft.fillRect(0, 0, SCREEN_W, STATUS_BAR_H, COL_STATUS_BG);
    tft.setTextColor(COL_DIM);
    tft.setTextSize(TEXT_STATUS);
    tft.setCursor(MARGIN, 6);
    switch (currentMode) {
        case MODE_CALCULATOR: tft.print("CALC"); break;
        case MODE_MENU:       tft.print("MENU"); break;
        case MODE_GRAPH:
        case MODE_GRAPH_INPUT: tft.print("GRAPH"); break;
        case MODE_SOLVER:
        case MODE_SOLVER_INPUT: tft.print("SOLVER"); break;
    }
    // Right side hint
    tft.setCursor(SCREEN_W - 60, 6);
    tft.print("[ESC]Menu");
}

// --- Calculator screen (partial redraw) ---
void drawCalcScreen() {
    unsigned long now = millis();
    if (now - lastBlink > 530) {
        cursorVisible = !cursorVisible;
        lastBlink = now;
    }

    if (needsFullRedraw) {
        tft.fillRect(0, STATUS_BAR_H, SCREEN_W, SCREEN_H - STATUS_BAR_H, COL_BG);
        drawStatusBar();
        needsFullRedraw = false;
        prevInputBuf = "";
        prevLineCount = 0;
    }

    int areaTop = STATUS_BAR_H + 4;
    int areaBottom = SCREEN_H - LINE_H_MAIN - 8; // Reserve space for input

    // Draw history (only if changed)
    if (prevLineCount != lines.size()) {
        tft.fillRect(0, areaTop, SCREEN_W, areaBottom - areaTop, COL_BG);

        int maxVisible = (areaBottom - areaTop) / LINE_H_HIST;
        int startIdx = 0;
        if ((int)lines.size() > maxVisible)
            startIdx = lines.size() - maxVisible;

        int y = areaBottom - ((int)lines.size() - startIdx) * LINE_H_HIST;
        if (y < areaTop) y = areaTop;

        tft.setTextSize(TEXT_HIST);
        for (size_t i = startIdx; i < lines.size(); i++) {
            if (y > areaBottom) break;

            if (lines[i].isExpr)
                tft.setTextColor(COL_TEXT);
            else if (lines[i].isError)
                tft.setTextColor(COL_ERROR);
            else
                tft.setTextColor(COL_ACCENT);

            int tw = lines[i].text.length() * CHAR_W_HIST;
            tft.setCursor(SCREEN_W - MARGIN - tw, y);
            tft.print(lines[i].text);
            y += LINE_H_HIST;
        }
        prevLineCount = lines.size();
    }

    // Draw input line (always, for cursor blink) — clear only input area
    int inputY = SCREEN_H - LINE_H_MAIN - 4;
    tft.fillRect(0, inputY, SCREEN_W, LINE_H_MAIN + 4, COL_BG);

    // Separator line
    tft.drawFastHLine(MARGIN, inputY - 2, SCREEN_W - 2 * MARGIN, COL_DIM);

    tft.setTextSize(TEXT_MAIN);
    tft.setTextColor(COL_TEXT);

    // Right-align input
    int maxChars = (SCREEN_W - 2 * MARGIN) / CHAR_W_MAIN;
    String displayInput = inputBuf;
    if ((int)displayInput.length() > maxChars)
        displayInput = displayInput.substring(displayInput.length() - maxChars);

    int tw = displayInput.length() * CHAR_W_MAIN;
    int cx = SCREEN_W - MARGIN - tw;
    tft.setCursor(cx, inputY + 2);
    tft.print(displayInput);

    // Blinking cursor
    if (cursorVisible) {
        int cursorX = SCREEN_W - MARGIN + 2;
        tft.fillRect(cursorX, inputY + 2, 3, CHAR_H_MAIN, COL_ACCENT);
    }

    prevInputBuf = inputBuf;
}

// --- Menu screen ---
void drawMenuScreen() {
    tft.fillScreen(COL_BG);
    drawStatusBar();

    tft.setTextSize(TEXT_MAIN);
    int startY = 60;

    for (int i = 0; i < MENU_ITEMS; i++) {
        int y = startY + i * (LINE_H_MAIN + 12);

        if (i == menuSelection) {
            tft.fillRect(MARGIN, y - 4, SCREEN_W - 2 * MARGIN, LINE_H_MAIN + 8, COL_MENU_SEL);
            tft.setTextColor(COL_TEXT);
        } else {
            tft.setTextColor(COL_DIM);
        }

        tft.setCursor(MARGIN + 10, y);
        tft.print(i + 1);
        tft.print(". ");
        tft.print(menuLabels[i]);
    }
}

// --- Graph input screen ---
void drawGraphInputScreen() {
    tft.fillScreen(COL_BG);
    drawStatusBar();

    tft.setTextSize(TEXT_HIST);
    tft.setTextColor(COL_DIM);
    tft.setCursor(MARGIN, 40);
    tft.print("Enter f(x):");

    tft.setTextSize(TEXT_MAIN);
    tft.setTextColor(COL_TEXT);
    tft.setCursor(MARGIN, 70);
    tft.print("y = ");
    tft.print(graphExpr);
    if (cursorVisible) tft.print("_");

    tft.setTextSize(TEXT_STATUS);
    tft.setTextColor(COL_DIM);
    tft.setCursor(MARGIN, SCREEN_H - 16);
    tft.print("Enter=Plot  ESC=Back  e.g. x*x or 2*x+1");
}

// --- Graph plot screen ---
void drawGraphPlot() {
    tft.fillScreen(COL_BG);
    drawStatusBar();

    int gTop = STATUS_BAR_H + 2;
    int gH = SCREEN_H - gTop;

    // Map graph coordinates to pixels
    auto mapX = [&](float x) -> int {
        return (int)((x - graphXmin) / (graphXmax - graphXmin) * SCREEN_W);
    };
    auto mapY = [&](float y) -> int {
        return gTop + (int)((graphYmax - y) / (graphYmax - graphYmin) * gH);
    };

    // Draw axes
    int xAxisY = mapY(0);
    int yAxisX = mapX(0);
    if (xAxisY >= gTop && xAxisY < SCREEN_H)
        tft.drawFastHLine(0, xAxisY, SCREEN_W, COL_AXES);
    if (yAxisX >= 0 && yAxisX < SCREEN_W)
        tft.drawFastVLine(yAxisX, gTop, gH, COL_AXES);

    // Axis labels
    tft.setTextSize(TEXT_STATUS);
    tft.setTextColor(COL_DIM);
    if (xAxisY >= gTop && xAxisY < SCREEN_H - 10) {
        tft.setCursor(SCREEN_W - 18, xAxisY - 10);
        tft.print("x");
    }
    if (yAxisX >= 0 && yAxisX < SCREEN_W - 10) {
        tft.setCursor(yAxisX + 4, gTop + 2);
        tft.print("y");
    }

    // Tick marks
    for (int i = (int)ceil(graphXmin); i <= (int)floor(graphXmax); i++) {
        if (i == 0) continue;
        int px = mapX(i);
        if (px >= 0 && px < SCREEN_W && xAxisY >= gTop && xAxisY < SCREEN_H) {
            tft.drawFastVLine(px, xAxisY - 2, 5, COL_DIM);
        }
    }
    for (int i = (int)ceil(graphYmin); i <= (int)floor(graphYmax); i++) {
        if (i == 0) continue;
        int py = mapY(i);
        if (py >= gTop && py < SCREEN_H && yAxisX >= 0 && yAxisX < SCREEN_W) {
            tft.drawFastHLine(yAxisX - 2, py, 5, COL_DIM);
        }
    }

    // Plot function
    evaluator.useX = true;
    int prevPx = -1, prevPy = -1;
    for (int px = 0; px < SCREEN_W; px++) {
        float x = graphXmin + (float)px / SCREEN_W * (graphXmax - graphXmin);
        evaluator.xVal = x;
        EvalResult r = evaluator.evaluate(graphExpr);
        if (r.ok) {
            int py = mapY(r.value);
            if (py >= gTop && py < SCREEN_H) {
                if (prevPx >= 0 && prevPy >= gTop && prevPy < SCREEN_H &&
                    abs(py - prevPy) < gH) {
                    tft.drawLine(prevPx, prevPy, px, py, COL_GRAPH);
                } else {
                    tft.drawPixel(px, py, COL_GRAPH);
                }
                prevPy = py;
            } else {
                prevPy = -1;
            }
            prevPx = px;
        } else {
            prevPx = -1;
            prevPy = -1;
        }
    }
    evaluator.useX = false;

    // Show expression at top
    tft.setTextSize(TEXT_STATUS);
    tft.setTextColor(COL_ACCENT);
    tft.setCursor(MARGIN, gTop + 2);
    tft.print("y=");
    tft.print(graphExpr);

    tft.setTextColor(COL_DIM);
    tft.setCursor(MARGIN, SCREEN_H - 10);
    tft.print("ESC=Back");
}

// --- Solver screen ---
void drawSolverScreen() {
    tft.fillScreen(COL_BG);
    drawStatusBar();

    tft.setTextSize(TEXT_HIST);
    tft.setTextColor(COL_TEXT);

    if (solverStep == 0) {
        // Choose type
        tft.setCursor(MARGIN, 40);
        tft.print("Equation Solver");

        tft.setTextSize(TEXT_MAIN);
        int y = 80;
        // Linear
        if (menuSelection == 0) {
            tft.fillRect(MARGIN, y - 4, SCREEN_W - 2 * MARGIN, LINE_H_MAIN + 8, COL_MENU_SEL);
            tft.setTextColor(COL_TEXT);
        } else {
            tft.setTextColor(COL_DIM);
        }
        tft.setCursor(MARGIN + 10, y);
        tft.print("1. ax+b=0");

        y += LINE_H_MAIN + 12;
        if (menuSelection == 1) {
            tft.fillRect(MARGIN, y - 4, SCREEN_W - 2 * MARGIN, LINE_H_MAIN + 8, COL_MENU_SEL);
            tft.setTextColor(COL_TEXT);
        } else {
            tft.setTextColor(COL_DIM);
        }
        tft.setCursor(MARGIN + 10, y);
        tft.print("2. ax^2+bx+c=0");

    } else if (solverStep >= 1 && solverStep <= 3) {
        // Input coefficients
        tft.setCursor(MARGIN, 40);
        if (solverQuadratic)
            tft.print("ax^2 + bx + c = 0");
        else
            tft.print("ax + b = 0");

        tft.setTextSize(TEXT_MAIN);
        tft.setTextColor(COL_ACCENT);
        tft.setCursor(MARGIN, 80);

        const char* labels[] = { "", "a", "b", "c" };
        tft.print(labels[solverStep]);
        tft.print(" = ");
        tft.setTextColor(COL_TEXT);
        tft.print(solverInput);
        if (cursorVisible) tft.print("_");

        // Show already entered values
        tft.setTextSize(TEXT_HIST);
        tft.setTextColor(COL_DIM);
        int y = 130;
        if (solverStep > 1) {
            tft.setCursor(MARGIN, y);
            tft.print("a = ");
            tft.print(formatNumber(solverA));
            y += LINE_H_HIST;
        }
        if (solverStep > 2) {
            tft.setCursor(MARGIN, y);
            tft.print("b = ");
            tft.print(formatNumber(solverB));
        }

    } else if (solverStep == 4) {
        // Show result
        tft.setCursor(MARGIN, 40);
        if (solverQuadratic)
            tft.print("ax^2 + bx + c = 0");
        else
            tft.print("ax + b = 0");

        tft.setTextSize(TEXT_HIST);
        tft.setTextColor(COL_DIM);
        int y = 70;
        tft.setCursor(MARGIN, y);
        tft.print("a="); tft.print(formatNumber(solverA));
        tft.print("  b="); tft.print(formatNumber(solverB));
        if (solverQuadratic) {
            tft.print("  c="); tft.print(formatNumber(solverC));
        }

        tft.setTextSize(TEXT_MAIN);
        tft.setTextColor(COL_ACCENT);
        tft.setCursor(MARGIN, 110);
        tft.print(solverResult);

        tft.setTextSize(TEXT_STATUS);
        tft.setTextColor(COL_DIM);
        tft.setCursor(MARGIN, SCREEN_H - 12);
        tft.print("ESC=Back  Enter=New");
    }
}

// ============================================================
//  SOLVER LOGIC
// ============================================================
void solveCurrent() {
    if (!solverQuadratic) {
        // Linear: ax + b = 0 -> x = -b/a
        if (fabs(solverA) < 1e-15) {
            solverResult = (fabs(solverB) < 1e-15) ? "Infinite" : "No solution";
        } else {
            double x = -solverB / solverA;
            solverResult = "x = " + formatNumber(x);
        }
    } else {
        // Quadratic: ax^2 + bx + c = 0
        if (fabs(solverA) < 1e-15) {
            // Degenerate to linear
            if (fabs(solverB) < 1e-15) {
                solverResult = (fabs(solverC) < 1e-15) ? "Infinite" : "No solution";
            } else {
                double x = -solverC / solverB;
                solverResult = "x = " + formatNumber(x);
            }
        } else {
            double disc = solverB * solverB - 4.0 * solverA * solverC;
            if (disc > 0) {
                double x1 = (-solverB + sqrt(disc)) / (2.0 * solverA);
                double x2 = (-solverB - sqrt(disc)) / (2.0 * solverA);
                solverResult = formatNumber(x1) + ", " + formatNumber(x2);
            } else if (fabs(disc) < 1e-12) {
                double x1 = -solverB / (2.0 * solverA);
                solverResult = "x = " + formatNumber(x1);
            } else {
                double re = -solverB / (2.0 * solverA);
                double im = sqrt(-disc) / (2.0 * solverA);
                solverResult = formatNumber(re) + "+-" + formatNumber(fabs(im)) + "i";
            }
        }
    }
    solverStep = 4;
}

// ============================================================
//  INPUT HANDLING
// ============================================================
// CardKB arrow key codes
#define KEY_UP    0xB5
#define KEY_DOWN  0xB6
#define KEY_LEFT  0xB4
#define KEY_RIGHT 0xB7

void handleInput(char key) {
    switch (currentMode) {

    case MODE_CALCULATOR:
        if (key == 0x1B) { // ESC -> menu
            currentMode = MODE_MENU;
            menuSelection = 0;
            needsFullRedraw = true;
        } else if (key == 0x08) { // Backspace
            if (inputBuf.length() > 0) inputBuf.remove(inputBuf.length() - 1);
        } else if (key == 0x0D) { // Enter
            doEvaluate();
        } else if (key >= 0x20 && key <= 0x7E && inputBuf.length() < 60) {
            inputBuf += key;
        }
        break;

    case MODE_MENU:
        if (key == 0x1B) { // ESC -> back to calc
            currentMode = MODE_CALCULATOR;
            needsFullRedraw = true;
        } else if (key == (char)KEY_UP || key == (char)KEY_LEFT) {
            menuSelection = (menuSelection - 1 + MENU_ITEMS) % MENU_ITEMS;
            needsFullRedraw = true;
        } else if (key == (char)KEY_DOWN || key == (char)KEY_RIGHT) {
            menuSelection = (menuSelection + 1) % MENU_ITEMS;
            needsFullRedraw = true;
        } else if (key == 0x0D) { // Enter
            if (menuSelection == 0) {
                currentMode = MODE_CALCULATOR;
            } else if (menuSelection == 1) {
                currentMode = MODE_GRAPH_INPUT;
                graphExpr = "";
            } else if (menuSelection == 2) {
                currentMode = MODE_SOLVER;
                solverStep = 0;
                menuSelection = 0;
            }
            needsFullRedraw = true;
        } else if (key == '1') {
            currentMode = MODE_CALCULATOR;
            needsFullRedraw = true;
        } else if (key == '2') {
            currentMode = MODE_GRAPH_INPUT;
            graphExpr = "";
            needsFullRedraw = true;
        } else if (key == '3') {
            currentMode = MODE_SOLVER;
            solverStep = 0;
            menuSelection = 0;
            needsFullRedraw = true;
        }
        break;

    case MODE_GRAPH_INPUT:
        if (key == 0x1B) {
            currentMode = MODE_MENU;
            menuSelection = 1;
            needsFullRedraw = true;
        } else if (key == 0x08) {
            if (graphExpr.length() > 0) graphExpr.remove(graphExpr.length() - 1);
            needsFullRedraw = true;
        } else if (key == 0x0D && graphExpr.length() > 0) {
            currentMode = MODE_GRAPH;
            needsFullRedraw = true;
        } else if (key >= 0x20 && key <= 0x7E && graphExpr.length() < 40) {
            graphExpr += key;
            needsFullRedraw = true;
        }
        break;

    case MODE_GRAPH:
        if (key == 0x1B) {
            currentMode = MODE_GRAPH_INPUT;
            needsFullRedraw = true;
        }
        break;

    case MODE_SOLVER:
        if (key == 0x1B) {
            currentMode = MODE_MENU;
            menuSelection = 2;
            needsFullRedraw = true;
        } else if (solverStep == 0) {
            if (key == (char)KEY_UP || key == (char)KEY_DOWN) {
                menuSelection = 1 - menuSelection;
                needsFullRedraw = true;
            } else if (key == 0x0D) {
                solverQuadratic = (menuSelection == 1);
                solverStep = 1;
                solverInput = "";
                solverA = solverB = solverC = 0;
                currentMode = MODE_SOLVER_INPUT;
                needsFullRedraw = true;
            } else if (key == '1') {
                solverQuadratic = false;
                solverStep = 1;
                solverInput = "";
                solverA = solverB = solverC = 0;
                currentMode = MODE_SOLVER_INPUT;
                needsFullRedraw = true;
            } else if (key == '2') {
                solverQuadratic = true;
                solverStep = 1;
                solverInput = "";
                solverA = solverB = solverC = 0;
                currentMode = MODE_SOLVER_INPUT;
                needsFullRedraw = true;
            }
        } else if (solverStep == 4) {
            if (key == 0x0D) {
                solverStep = 0;
                menuSelection = 0;
                needsFullRedraw = true;
            }
        }
        break;

    case MODE_SOLVER_INPUT:
        if (key == 0x1B) {
            currentMode = MODE_SOLVER;
            solverStep = 0;
            menuSelection = 0;
            needsFullRedraw = true;
        } else if (key == 0x08) {
            if (solverInput.length() > 0) solverInput.remove(solverInput.length() - 1);
            needsFullRedraw = true;
        } else if (key == 0x0D && solverInput.length() > 0) {
            double val = atof(solverInput.c_str());
            if (solverStep == 1) {
                solverA = val;
                solverStep = 2;
                solverInput = "";
            } else if (solverStep == 2) {
                solverB = val;
                if (solverQuadratic) {
                    solverStep = 3;
                    solverInput = "";
                } else {
                    solveCurrent();
                    currentMode = MODE_SOLVER;
                }
            } else if (solverStep == 3) {
                solverC = val;
                solveCurrent();
                currentMode = MODE_SOLVER;
            }
            needsFullRedraw = true;
        } else if ((key >= '0' && key <= '9') || key == '.' || key == '-') {
            if (solverInput.length() < 20) {
                solverInput += key;
                needsFullRedraw = true;
            }
        }
        break;
    }
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("Nash Calculator v1.0");

    // Backlight on
    pinMode(TFT_BLK, OUTPUT);
    digitalWrite(TFT_BLK, HIGH);

    // Remap default SPI to our pins and init display
    SPI.begin(TFT_CLK, -1, TFT_MOSI, TFT_CS);
    tft.begin();
    tft.setSPISpeed(40000000);  // 40MHz hardware SPI
    tft.setRotation(1);
    tft.fillScreen(COL_BG);
    Serial.println("ILI9341 initialized (HW SPI 40MHz, 320x240)");

    // Keyboard init
    cardkb.begin();

    needsFullRedraw = true;
    drawStatusBar();
    drawCalcScreen();
    Serial.println("Setup complete");
}

// ============================================================
//  MAIN LOOP
// ============================================================
void loop() {
    char key = cardkb.read();

    if (key != '\0') {
        Serial.printf("Key: 0x%02X '%c'\n", key, key);
        handleInput(key);
    }

    // Cursor blink timing
    unsigned long now = millis();
    if (now - lastBlink > 530) {
        cursorVisible = !cursorVisible;
        lastBlink = now;
    }

    // Draw current mode
    switch (currentMode) {
        case MODE_CALCULATOR:
            drawCalcScreen();
            break;
        case MODE_MENU:
            if (needsFullRedraw) { drawMenuScreen(); needsFullRedraw = false; }
            break;
        case MODE_GRAPH_INPUT:
            if (needsFullRedraw) { drawGraphInputScreen(); needsFullRedraw = false; }
            break;
        case MODE_GRAPH:
            if (needsFullRedraw) { drawGraphPlot(); needsFullRedraw = false; }
            break;
        case MODE_SOLVER:
        case MODE_SOLVER_INPUT:
            if (needsFullRedraw) { drawSolverScreen(); needsFullRedraw = false; }
            break;
    }

    delay(5);
}
