// ============================================================
//  Nash's DIY Graphing Calculator — Single-File Firmware
//  MCU: Seeed XIAO ESP32S3
//  Display: ILI9341 240×320 SPI (landscape 320×240)
//  Input: M5Stack CardKB (I2C)
//  No sprites, no framebuffer — direct drawing only
// ============================================================

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>
#include <vector>

// ============================================================
//  PIN DEFINITIONS (proven working from TEST_script.ino)
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
constexpr uint16_t COL_BG        = 0x0000;  // Black
constexpr uint16_t COL_TEXT      = 0xFFFF;  // White
constexpr uint16_t COL_DIM       = 0x7BEF;  // Gray
constexpr uint16_t COL_ACCENT    = 0x07FF;  // Cyan
constexpr uint16_t COL_STATUS_BG = 0x0841;  // Very dark gray
constexpr uint16_t COL_ERROR     = 0xF800;  // Red

static const int LINE_H = 16;
static const int MARGIN = 6;

// ============================================================
//  DISPLAY OBJECT
// ============================================================
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST);

// ============================================================
//  EXPRESSION EVALUATOR — recursive descent parser
//  Supports: + - * / ( ) decimal numbers, unary minus
// ============================================================
struct EvalResult {
    bool ok;
    double value;
    String error;
};

class Evaluator {
public:
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

    void skipSpaces() {
        while (*_p == ' ') _p++;
    }

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
        double val = parseFactor();
        skipSpaces();
        while (*_p == '*' || *_p == '/') {
            char op = *_p++;
            double right = parseFactor();
            if (op == '*') val *= right; else val /= right;
            skipSpaces();
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
//  CARDKB INPUT — I2C keyboard driver
// ============================================================
class CardKB {
public:
    void begin() {
        Wire.begin(KB_SDA, KB_SCL);
        Wire.setClock(100000);
        Wire.beginTransmission(KB_ADDR);
        _connected = (Wire.endTransmission() == 0);
        Serial.println(_connected ? "CardKB detected at 0x5F" : "CardKB NOT found");
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
//  DISPLAY LINE DATA
// ============================================================
struct DisplayLine {
    String text;
    bool isExpr;
    bool isError;
};

// ============================================================
//  CALCULATOR STATE
// ============================================================
String inputBuf;
String lastAnswer;
std::vector<DisplayLine> lines;
Evaluator evaluator;
CardKB cardkb;
bool cursorVisible = true;
unsigned long lastBlink = 0;

// ============================================================
//  NUMBER FORMATTING
// ============================================================
String formatNumber(double v) {
    char buf[32];
    if (fabs(v) > 1e12 || (fabs(v) < 1e-6 && v != 0)) {
        snprintf(buf, sizeof(buf), "%.8e", v);
    } else {
        snprintf(buf, sizeof(buf), "%.10f", v);
        // Trim trailing zeros after decimal
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
//  EVALUATE INPUT EXPRESSION
// ============================================================
void doEvaluate() {
    if (inputBuf.length() == 0) return;

    DisplayLine exprLine;
    exprLine.text = inputBuf;
    exprLine.isExpr = true;
    exprLine.isError = false;
    lines.push_back(exprLine);

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

    // Cap history
    while (lines.size() > 200)
        lines.erase(lines.begin());
}

// ============================================================
//  INPUT HANDLING
// ============================================================
void handleInput(char key) {
    switch (key) {
        case 0x08:  // Backspace
            if (inputBuf.length() > 0)
                inputBuf.remove(inputBuf.length() - 1);
            break;
        case 0x0D:  // Enter
            doEvaluate();
            break;
        case 0x1B:  // Escape — clear input or history
            if (inputBuf.length() > 0) inputBuf = "";
            else lines.clear();
            break;
        default:
            if (key >= 0x20 && key <= 0x7E && inputBuf.length() < 60)
                inputBuf += key;
            break;
    }
}

// ============================================================
//  SCREEN DRAWING — direct to ILI9341, no framebuffer
// ============================================================
void drawStatusBar() {
    tft.fillRect(0, 0, SCREEN_W, STATUS_BAR_H, COL_STATUS_BG);
    tft.setTextColor(COL_DIM);
    tft.setTextSize(1);
    tft.setCursor(MARGIN, 6);
    tft.print("CALC");
}

void drawScreen() {
    unsigned long now = millis();
    if (now - lastBlink > 530) {
        cursorVisible = !cursorVisible;
        lastBlink = now;
    }

    tft.fillScreen(COL_BG);
    drawStatusBar();

    int areaTop = STATUS_BAR_H + 2;
    int areaBottom = SCREEN_H - 4;

    // Bottom-align lines
    int totalLines = lines.size() + 1;
    int y = areaBottom - totalLines * LINE_H;
    if (y < areaTop) y = areaTop;

    tft.setTextSize(1);

    // History lines (right-aligned)
    for (size_t i = 0; i < lines.size(); i++) {
        if (y + LINE_H < areaTop) { y += LINE_H; continue; }
        if (y > areaBottom) break;

        if (lines[i].isExpr)
            tft.setTextColor(COL_TEXT);
        else if (lines[i].isError)
            tft.setTextColor(COL_ERROR);
        else
            tft.setTextColor(COL_ACCENT);

        int tw = lines[i].text.length() * 6;
        tft.setCursor(SCREEN_W - MARGIN - tw, y);
        tft.print(lines[i].text);
        y += LINE_H;
    }

    // Current input line with blinking cursor
    if (y <= areaBottom) {
        tft.setTextColor(COL_TEXT);
        int tw = inputBuf.length() * 6;
        int cx = SCREEN_W - MARGIN - tw;
        tft.setCursor(cx, y);
        tft.print(inputBuf);

        if (cursorVisible) {
            int cursorX = SCREEN_W - MARGIN;
            tft.fillRect(cursorX + 1, y, 2, LINE_H - 2, COL_ACCENT);
        }
    }
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("Nash Calculator v0.3 (ILI9341)");

    // Backlight on
    pinMode(TFT_BLK, OUTPUT);
    digitalWrite(TFT_BLK, HIGH);

    // Display init
    tft.begin();
    tft.setRotation(1);  // Landscape 320×240 (90° CW)
    tft.fillScreen(COL_BG);
    Serial.println("ILI9341 display initialized (320x240 landscape)");

    // Keyboard init
    cardkb.begin();

    // Initial draw
    drawScreen();
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
        drawScreen();
    }

    // Periodic redraw for cursor blink
    static unsigned long lastRedraw = 0;
    unsigned long now = millis();
    if (now - lastRedraw > 500) {
        lastRedraw = now;
        drawScreen();
    }

    delay(5);
}
