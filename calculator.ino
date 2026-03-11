// ============================================================
//  Nash's DIY Graphing Calculator — TI-84 Plus CE Style
//  MCU: Seeed XIAO ESP32S3
//  Display: ILI9341 240×320 SPI (landscape 320×240)
//  Input: M5Stack CardKB (I2C)
//  Hardware SPI @ 40MHz
// ============================================================

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>
#include <vector>

// ============ CONFIGURATION ============

#define TFT_CS    2
#define TFT_DC    3
#define TFT_RST   4
#define TFT_MOSI  9
#define TFT_CLK   7
#define TFT_BLK   5
#define KB_SDA    1
#define KB_SCL    6
#define KB_ADDR   0x5F

constexpr int SW = 320;
constexpr int SH = 240;
constexpr int SBAR_H = 14;
constexpr unsigned long KB_POLL_MS = 20;

// Colors
constexpr uint16_t C_BG      = 0x0000;
constexpr uint16_t C_FG      = 0xFFFF;
constexpr uint16_t C_DIM     = 0x7BEF;
constexpr uint16_t C_CYAN    = 0x07FF;
constexpr uint16_t C_YELLOW  = 0xFFE0;
constexpr uint16_t C_GREEN   = 0x07E0;
constexpr uint16_t C_RED     = 0xF800;
constexpr uint16_t C_BLUE    = 0x001F;
constexpr uint16_t C_SBAR    = 0x0841;
constexpr uint16_t C_AXES    = 0x4208;
constexpr uint16_t C_SEL     = 0x001F;

// Graph colors for Y1-Y4
const uint16_t graphColors[] = { C_FG, C_CYAN, C_YELLOW, C_GREEN };

// CardKB keys
#define KEY_UP    0xB5
#define KEY_DOWN  0xB6
#define KEY_LEFT  0xB4
#define KEY_RIGHT 0xB7
#define KEY_ESC   0x1B
#define KEY_BS    0x08
#define KEY_ENTER 0x0D

// App modes
enum Mode {
    M_CALC, M_MENU, M_YEDIT, M_GRAPH, M_TABLE, M_SOLVER, M_CALCULUS,
    M_GRAPH_WIN, M_TABLE_SET, M_SOLVER_EQ, M_CALC_SUB
};

Mode mode = M_CALC;
bool dirty = true;  // screen needs redraw

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);

// ============ EVALUATOR ============
// Recursive descent: + - * / ^ ( ) numbers, x variable, pi

class Eval {
public:
    double xVal = 0;
    bool allowX = false;

    struct R { bool ok; double v; };

    R eval(const String& e) {
        p = e.c_str();
        double v = pExpr();
        skip();
        if (*p) return {false, 0};
        if (isnan(v) || isinf(v)) return {false, 0};
        return {true, v};
    }

    // Evaluate with x
    double f(const String& e, double x) {
        allowX = true; xVal = x;
        auto r = eval(e);
        allowX = false;
        return r.ok ? r.v : NAN;
    }

private:
    const char* p;
    void skip() { while (*p == ' ') p++; }

    double pExpr() {
        double v = pTerm();
        skip();
        while (*p == '+' || *p == '-') {
            char o = *p++; double r = pTerm();
            v = (o == '+') ? v + r : v - r;
            skip();
        }
        return v;
    }

    double pTerm() {
        double v = pPow();
        skip();
        while (*p == '*' || *p == '/') {
            char o = *p++; double r = pPow();
            v = (o == '*') ? v * r : v / r;
            skip();
        }
        return v;
    }

    double pPow() {
        double v = pUnary();
        skip();
        if (*p == '^') { p++; v = pow(v, pPow()); }
        return v;
    }

    double pUnary() {
        skip();
        if (*p == '-') { p++; return -pUnary(); }
        if (*p == '+') { p++; return pUnary(); }
        return pAtom();
    }

    double pAtom() {
        skip();
        if (*p == '(') {
            p++; double v = pExpr(); skip();
            if (*p == ')') p++;
            return v;
        }
        // functions: sin, cos, tan, sqrt, abs, ln, log
        if (matchWord("sin")) { p+=3; return sin(pAtom()); }
        if (matchWord("cos")) { p+=3; return cos(pAtom()); }
        if (matchWord("tan")) { p+=3; return tan(pAtom()); }
        if (matchWord("sqrt")) { p+=4; return sqrt(pAtom()); }
        if (matchWord("abs")) { p+=3; return fabs(pAtom()); }
        if (matchWord("ln")) { p+=2; return log(pAtom()); }
        if (matchWord("log")) { p+=3; return log10(pAtom()); }
        if (matchWord("pi")) { p+=2; return M_PI; }
        if (allowX && (*p == 'x' || *p == 'X')) { p++; return xVal; }
        return pNum();
    }

    bool matchWord(const char* w) {
        int n = strlen(w);
        if (strncmp(p, w, n) == 0 && !isalnum(p[n]) && p[n] != '_') return true;
        return false;
    }

    double pNum() {
        skip();
        const char* s = p;
        while (*p >= '0' && *p <= '9') p++;
        if (*p == '.') { p++; while (*p >= '0' && *p <= '9') p++; }
        if (p == s) return NAN;
        char b[32]; int n = min((int)(p-s), 30);
        memcpy(b, s, n); b[n] = 0;
        return atof(b);
    }
};

Eval ev;

// ============ SYMBOLIC MATH ============
// Polynomial representation for symbolic differentiation/integration

struct Term { double coeff; int exp; };

// Parse polynomial string into terms. Supports: ax^n, ax, a, x^n, x
// Returns empty vector on failure.
std::vector<Term> parsePoly(const String& s) {
    std::vector<Term> terms;
    const char* p = s.c_str();
    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;

        double sign = 1;
        if (*p == '+') { p++; }
        else if (*p == '-') { sign = -1; p++; }
        while (*p == ' ') p++;

        double coeff = 0;
        bool hasCoeff = false;
        const char* ns = p;
        while ((*p >= '0' && *p <= '9') || *p == '.') p++;
        if (p > ns) {
            char b[32]; int n = min((int)(p-ns),30);
            memcpy(b, ns, n); b[n]=0;
            coeff = atof(b);
            hasCoeff = true;
        }

        while (*p == ' ') p++;
        if (*p == 'x' || *p == 'X') {
            if (!hasCoeff) coeff = 1;
            p++;
            while (*p == ' ') p++;
            if (*p == '^') {
                p++;
                while (*p == ' ') p++;
                bool negExp = false;
                if (*p == '-') { negExp = true; p++; }
                const char* es = p;
                while (*p >= '0' && *p <= '9') p++;
                if (p > es) {
                    char b[16]; int n = min((int)(p-es),14);
                    memcpy(b, es, n); b[n]=0;
                    int e = atoi(b);
                    terms.push_back({sign*coeff, negExp ? -e : e});
                }
            } else {
                terms.push_back({sign*coeff, 1});
            }
        } else if (hasCoeff) {
            terms.push_back({sign*coeff, 0});
        } else {
            break; // parse error
        }
        while (*p == ' ') p++;
    }
    return terms;
}

String formatPoly(const std::vector<Term>& terms) {
    if (terms.empty()) return "0";
    String s;
    for (size_t i = 0; i < terms.size(); i++) {
        double c = terms[i].coeff;
        int e = terms[i].exp;
        if (i > 0 && c >= 0) s += "+";
        if (e == 0) {
            char b[20]; snprintf(b, sizeof(b), "%.4g", c); s += b;
        } else if (e == 1) {
            if (fabs(c - 1) < 1e-12) s += "x";
            else if (fabs(c + 1) < 1e-12) s += "-x";
            else { char b[20]; snprintf(b, sizeof(b), "%.4g", c); s += b; s += "x"; }
        } else {
            if (fabs(c - 1) < 1e-12) { s += "x^"; s += String(e); }
            else if (fabs(c + 1) < 1e-12) { s += "-x^"; s += String(e); }
            else { char b[20]; snprintf(b, sizeof(b), "%.4g", c); s += b; s += "x^"; s += String(e); }
        }
    }
    return s;
}

String symbolicDerivative(const String& expr) {
    auto terms = parsePoly(expr);
    if (terms.empty()) return "Error";
    std::vector<Term> result;
    for (auto& t : terms) {
        if (t.exp != 0) {
            result.push_back({t.coeff * t.exp, t.exp - 1});
        }
    }
    if (result.empty()) return "0";
    return formatPoly(result);
}

String symbolicIntegral(const String& expr) {
    auto terms = parsePoly(expr);
    if (terms.empty()) return "Error";
    std::vector<Term> result;
    for (auto& t : terms) {
        int ne = t.exp + 1;
        if (ne == 0) return "Error: ln term";
        result.push_back({t.coeff / ne, ne});
    }
    return formatPoly(result) + "+C";
}

double numericDerivative(const String& expr, double x0) {
    double h = 1e-6;
    double fp = ev.f(expr, x0 + h);
    double fm = ev.f(expr, x0 - h);
    if (isnan(fp) || isnan(fm)) return NAN;
    return (fp - fm) / (2 * h);
}

double simpsonIntegral(const String& expr, double a, double b) {
    int n = 200; // must be even
    double h = (b - a) / n;
    double sum = ev.f(expr, a) + ev.f(expr, b);
    for (int i = 1; i < n; i++) {
        double x = a + i * h;
        double fx = ev.f(expr, x);
        if (isnan(fx)) return NAN;
        sum += (i % 2 == 0) ? 2 * fx : 4 * fx;
    }
    return sum * h / 3.0;
}

// ============ UI DRAWING ============

// CardKB reader
class CardKB {
public:
    void begin() {
        Wire.begin(KB_SDA, KB_SCL);
        Wire.setClock(100000);
        Wire.beginTransmission(KB_ADDR);
        ok = (Wire.endTransmission() == 0);
    }
    char read() {
        unsigned long now = millis();
        if (now - last < KB_POLL_MS) return 0;
        last = now;
        if (!ok) return 0;
        Wire.requestFrom((uint8_t)KB_ADDR, (uint8_t)1);
        if (Wire.available()) { char c = Wire.read(); if (c) return c; }
        return 0;
    }
private:
    bool ok = false;
    unsigned long last = 0;
};

CardKB kb;

String fmtNum(double v) {
    char buf[32];
    if (fabs(v) > 1e12 || (fabs(v) < 1e-6 && v != 0)) {
        snprintf(buf, sizeof(buf), "%.6e", v);
    } else {
        snprintf(buf, sizeof(buf), "%.10f", v);
        char* d = strchr(buf, '.');
        if (d) { char* e = buf+strlen(buf)-1; while (e>d && *e=='0') *e--=0; if (e==d) *e=0; }
    }
    return String(buf);
}

void drawStatusBar(const char* label) {
    tft.fillRect(0, 0, SW, SBAR_H, C_SBAR);
    tft.setTextColor(C_DIM); tft.setTextSize(1);
    tft.setCursor(4, 3); tft.print(label);
    tft.setCursor(SW - 54, 3); tft.print("[ESC]Menu");
}

// ============ CALCULATOR MODE ============

struct HistLine { String text; bool isExpr; bool isErr; };
std::vector<HistLine> hist;
String inp;
int menuSel = 0;

void drawCalc() {
    tft.fillScreen(C_BG);
    drawStatusBar("CALCULATE");

    int areaTop = SBAR_H + 2;
    int inputY = SH - 28;

    // History: right-aligned, scrolling up
    int lineH = 18;
    int maxVis = (inputY - 4 - areaTop) / lineH;
    int start = (int)hist.size() > maxVis ? hist.size() - maxVis : 0;
    int y = inputY - 4 - ((int)hist.size() - start) * lineH;
    if (y < areaTop) y = areaTop;

    tft.setTextSize(2);
    for (size_t i = start; i < hist.size(); i++) {
        if (y >= inputY - 4) break;
        tft.setTextColor(hist[i].isErr ? C_RED : hist[i].isExpr ? C_FG : C_CYAN);
        int tw = hist[i].text.length() * 12;
        tft.setCursor(SW - 6 - tw, y);
        tft.print(hist[i].text);
        y += lineH;
    }

    // Separator
    tft.drawFastHLine(4, inputY - 2, SW - 8, C_DIM);

    // Input line right-aligned with cursor
    tft.setTextSize(3);
    tft.setTextColor(C_FG);
    int maxC = (SW - 12) / 18;
    String disp = inp;
    if ((int)disp.length() > maxC) disp = disp.substring(disp.length() - maxC);
    disp += "_";
    int tw = disp.length() * 18;
    tft.setCursor(SW - 6 - tw, inputY);
    tft.print(disp);
}

void doCalcEval() {
    if (inp.length() == 0) return;
    hist.push_back({inp, true, false});
    ev.allowX = false;
    auto r = ev.eval(inp);
    if (r.ok) {
        hist.push_back({fmtNum(r.v), false, false});
    } else {
        hist.push_back({"Error", false, true});
    }
    inp = "";
    while (hist.size() > 200) hist.erase(hist.begin());
    dirty = true;
}

// ============ Y= EDITOR ============

String yExprs[4] = {"", "", "", ""};
bool yActive[4] = {true, true, true, true};
int yEditSel = 0; // 0-3, which Y is selected
String yEditBuf;
bool yEditing = false;

void drawYEdit() {
    tft.fillScreen(C_BG);
    drawStatusBar("Y= EDITOR");

    tft.setTextSize(2);
    for (int i = 0; i < 4; i++) {
        int y = 24 + i * 44;
        uint16_t col = graphColors[i];

        if (i == yEditSel) {
            tft.fillRect(2, y - 2, SW - 4, 40, 0x0841);
        }

        tft.setTextColor(col);
        tft.setCursor(6, y);
        tft.print("Y"); tft.print(i + 1); tft.print("=");

        tft.setTextColor(C_FG);
        tft.setCursor(48, y);
        if (yEditing && i == yEditSel) {
            tft.print(yEditBuf);
            tft.print("_");
        } else {
            tft.print(yExprs[i].length() > 0 ? yExprs[i] : "");
        }

        // Active indicator
        if (yExprs[i].length() > 0) {
            tft.setTextColor(yActive[i] ? C_GREEN : C_DIM);
            tft.setCursor(SW - 18, y);
            tft.print(yActive[i] ? "*" : " ");
        }
    }

    tft.setTextSize(1);
    tft.setTextColor(C_DIM);
    tft.setCursor(4, SH - 12);
    tft.print("Enter=Edit  Arrows=Nav  ESC=Menu");
}

// ============ GRAPH MODE ============

float gXmin = -10, gXmax = 10, gYmin = -10, gYmax = 10;
bool traceOn = false;
int traceFn = 0;   // which Y function trace is on
float traceX = 0;

// Sub-mode for window input
int winField = 0;  // 0=xmin,1=xmax,2=ymin,3=ymax
String winBuf;

void drawGraph() {
    tft.fillScreen(C_BG);

    int gTop = SBAR_H;
    int gH = SH - gTop - (traceOn ? 16 : 0);

    auto mx = [&](float x) -> int { return (int)((x - gXmin) / (gXmax - gXmin) * SW); };
    auto my = [&](float y) -> int { return gTop + (int)((gYmax - y) / (gYmax - gYmin) * gH); };

    // Axes
    int ax = mx(0), ay = my(0);
    if (ay >= gTop && ay < gTop + gH) tft.drawFastHLine(0, ay, SW, C_AXES);
    if (ax >= 0 && ax < SW) tft.drawFastVLine(ax, gTop, gH, C_AXES);

    // Tick marks
    tft.setTextSize(1); tft.setTextColor(C_DIM);
    float xStep = max(1.0f, (gXmax - gXmin) / 20.0f);
    xStep = pow(10, floor(log10(xStep)));
    for (float t = ceil(gXmin/xStep)*xStep; t <= gXmax; t += xStep) {
        if (fabs(t) < xStep*0.1) continue;
        int px = mx(t);
        if (px >= 0 && px < SW && ay >= gTop && ay < gTop+gH) {
            tft.drawFastVLine(px, ay-2, 5, C_DIM);
        }
    }
    float yStep = max(1.0f, (gYmax - gYmin) / 15.0f);
    yStep = pow(10, floor(log10(yStep)));
    for (float t = ceil(gYmin/yStep)*yStep; t <= gYmax; t += yStep) {
        if (fabs(t) < yStep*0.1) continue;
        int py = my(t);
        if (py >= gTop && py < gTop+gH && ax >= 0 && ax < SW) {
            tft.drawFastHLine(ax-2, py, 5, C_DIM);
        }
    }

    // Axis labels
    tft.setTextColor(C_DIM);
    char lb[16];
    snprintf(lb, 16, "%.4g", gXmin); tft.setCursor(2, gTop+gH-10); tft.print(lb);
    snprintf(lb, 16, "%.4g", gXmax); tft.setCursor(SW-40, gTop+gH-10); tft.print(lb);
    snprintf(lb, 16, "%.4g", gYmax); tft.setCursor(2, gTop+2); tft.print(lb);
    snprintf(lb, 16, "%.4g", gYmin); tft.setCursor(2, gTop+gH-20); tft.print(lb);

    // Plot Y1-Y4
    ev.allowX = true;
    for (int fn = 0; fn < 4; fn++) {
        if (!yActive[fn] || yExprs[fn].length() == 0) continue;
        int prevPx = -1, prevPy = -1;
        for (int px = 0; px < SW; px++) {
            float x = gXmin + (float)px / SW * (gXmax - gXmin);
            double y = ev.f(yExprs[fn], x);
            if (!isnan(y)) {
                int py = my(y);
                if (py >= gTop && py < gTop+gH) {
                    if (prevPx >= 0 && prevPy >= gTop && prevPy < gTop+gH && abs(py-prevPy) < gH) {
                        tft.drawLine(prevPx, prevPy, px, py, graphColors[fn]);
                    } else {
                        tft.drawPixel(px, py, graphColors[fn]);
                    }
                    prevPy = py;
                } else { prevPy = -1; }
                prevPx = px;
            } else { prevPx = -1; prevPy = -1; }
        }
    }
    ev.allowX = false;

    // Status bar on top of graph
    tft.fillRect(0, 0, SW, SBAR_H, C_SBAR);
    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setCursor(4, 3); tft.print("GRAPH");
    tft.setCursor(SW - 120, 3); tft.print("W=Win Z=Zoom T=Trace");

    // Trace cursor
    if (traceOn) {
        // Find active function for trace
        int fn = traceFn;
        if (fn >= 0 && fn < 4 && yActive[fn] && yExprs[fn].length() > 0) {
            double y = ev.f(yExprs[fn], traceX);
            if (!isnan(y)) {
                int px = mx(traceX), py = my(y);
                if (px >= 0 && px < SW && py >= gTop && py < gTop+gH) {
                    // Crosshair
                    tft.drawCircle(px, py, 4, C_FG);
                    tft.drawPixel(px, py, C_FG);
                }
            }
            // Bottom info bar
            int infoY = SH - 14;
            tft.fillRect(0, infoY, SW, 14, C_SBAR);
            tft.setTextSize(1); tft.setTextColor(C_FG);
            tft.setCursor(4, infoY + 3);
            char buf[64];
            snprintf(buf, 64, "Y%d  X=%.4f  Y=%.4f", fn+1, traceX, isnan(ev.f(yExprs[fn], traceX)) ? 0.0 : ev.f(yExprs[fn], traceX));
            tft.print(buf);
            tft.setCursor(SW - 60, infoY + 3);
            tft.print("Up/Dn=Fn");
        }
    }
}

void drawGraphWindow() {
    tft.fillScreen(C_BG);
    drawStatusBar("WINDOW");

    const char* labels[] = {"Xmin", "Xmax", "Ymin", "Ymax"};
    float* vals[] = {&gXmin, &gXmax, &gYmin, &gYmax};

    tft.setTextSize(2);
    for (int i = 0; i < 4; i++) {
        int y = 30 + i * 40;
        tft.setTextColor(i == winField ? C_CYAN : C_DIM);
        tft.setCursor(10, y);
        tft.print(labels[i]);
        tft.print("=");
        tft.setTextColor(C_FG);
        if (i == winField) {
            tft.print(winBuf); tft.print("_");
        } else {
            tft.print(fmtNum(*vals[i]));
        }
    }

    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setCursor(4, SH - 12);
    tft.print("Enter=Next  ESC=Cancel");
}

// ============ TABLE MODE ============

float tblStart = -5, tblStep = 1;
int tblScroll = 0;
bool tblSetMode = false;
int tblSetField = 0; // 0=start, 1=step
String tblSetBuf;

void drawTable() {
    tft.fillScreen(C_BG);
    drawStatusBar("TABLE");

    // Count active functions
    int activeFns[4], nActive = 0;
    for (int i = 0; i < 4; i++) {
        if (yActive[i] && yExprs[i].length() > 0) activeFns[nActive++] = i;
    }

    if (nActive == 0) {
        tft.setTextSize(2); tft.setTextColor(C_DIM);
        tft.setCursor(40, 100); tft.print("No Y= functions");
        tft.setCursor(40, 130); tft.print("Press ESC");
        return;
    }

    // Column widths
    int colW = (SW - 60) / max(nActive, 1);
    int rowH = 18;
    int headerY = SBAR_H + 2;
    int dataY = headerY + rowH + 2;
    int maxRows = (SH - dataY - 16) / rowH;

    // Header
    tft.setTextSize(1); tft.setTextColor(C_CYAN);
    tft.setCursor(4, headerY); tft.print("X");
    for (int i = 0; i < nActive; i++) {
        tft.setCursor(60 + i * colW, headerY);
        tft.print("Y"); tft.print(activeFns[i] + 1);
    }
    tft.drawFastHLine(0, headerY + rowH, SW, C_DIM);

    // Rows
    ev.allowX = true;
    tft.setTextSize(1);
    for (int r = 0; r < maxRows; r++) {
        float x = tblStart + (tblScroll + r) * tblStep;
        int y = dataY + r * rowH;

        tft.setTextColor(C_FG);
        char xb[16]; snprintf(xb, 16, "%.4g", x);
        tft.setCursor(4, y); tft.print(xb);

        for (int i = 0; i < nActive; i++) {
            double val = ev.f(yExprs[activeFns[i]], x);
            tft.setTextColor(graphColors[activeFns[i]]);
            tft.setCursor(60 + i * colW, y);
            if (isnan(val)) tft.print("undef");
            else { char vb[16]; snprintf(vb, 16, "%.4g", val); tft.print(vb); }
        }
    }
    ev.allowX = false;

    // Footer
    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setCursor(4, SH - 12);
    char fb[64];
    snprintf(fb, 64, "Start=%.4g Step=%.4g  S=Settings", tblStart, tblStep);
    tft.print(fb);
}

void drawTableSettings() {
    tft.fillScreen(C_BG);
    drawStatusBar("TABLE SETUP");

    const char* labels[] = {"TblStart", "TblStep"};
    float* vals[] = {&tblStart, &tblStep};

    tft.setTextSize(2);
    for (int i = 0; i < 2; i++) {
        int y = 60 + i * 50;
        tft.setTextColor(i == tblSetField ? C_CYAN : C_DIM);
        tft.setCursor(10, y);
        tft.print(labels[i]);
        tft.print("=");
        tft.setTextColor(C_FG);
        if (i == tblSetField) {
            tft.print(tblSetBuf); tft.print("_");
        } else {
            tft.print(fmtNum(*vals[i]));
        }
    }
    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setCursor(4, SH-12); tft.print("Enter=Next ESC=Cancel");
}

// ============ SOLVER MODE ============

// Solver has sub-modes: 0=type select, 1=equation input, 2=result
// Types: 0=equation solver(f(x)=g(x)), 1=linear(ax+b=0), 2=quadratic(ax^2+bx+c=0)
int solverType = 0;
int solverSel = 0;
int solverPhase = 0; // 0=select, 1=input, 2=result
String solverLHS, solverRHS;
String solverCoefBuf;
double sA, sB, sC;
int solverCoefStep = 0; // which coef we're entering
String solverResult;
bool solverEditingRHS = false;

// Newton's method to solve f(x)-g(x)=0
double solveEquation(const String& lhs, const String& rhs) {
    // Build h(x) = lhs - (rhs)
    String combined = "(" + lhs + ")-(" + rhs + ")";
    // Try bisection on [-100, 100] first to find sign change
    ev.allowX = true;
    double lo = -100, hi = 100;
    double flo = ev.f(combined, lo);
    double fhi = ev.f(combined, hi);

    // If no sign change, try Newton from x=0
    if (isnan(flo) || isnan(fhi) || flo * fhi > 0) {
        // Newton's method from x=0
        double x = 0;
        for (int i = 0; i < 200; i++) {
            double fx = ev.f(combined, x);
            if (isnan(fx)) { ev.allowX = false; return NAN; }
            if (fabs(fx) < 1e-12) { ev.allowX = false; return x; }
            double h = 1e-7;
            double dfx = (ev.f(combined, x+h) - ev.f(combined, x-h)) / (2*h);
            if (fabs(dfx) < 1e-15) { ev.allowX = false; return NAN; }
            x = x - fx / dfx;
        }
        ev.allowX = false;
        double fx = ev.f(combined, x);
        return (fabs(fx) < 1e-6) ? x : NAN;
    }

    // Bisection
    for (int i = 0; i < 200; i++) {
        double mid = (lo + hi) / 2;
        double fmid = ev.f(combined, mid);
        if (isnan(fmid)) break;
        if (fabs(fmid) < 1e-12) { ev.allowX = false; return mid; }
        if (flo * fmid < 0) { hi = mid; fhi = fmid; }
        else { lo = mid; flo = fmid; }
    }
    ev.allowX = false;
    return (lo + hi) / 2;
}

void drawSolver() {
    tft.fillScreen(C_BG);
    drawStatusBar("SOLVER");

    if (solverPhase == 0) {
        // Type selection
        const char* types[] = {"1. Equation: f(x)=g(x)", "2. Linear: ax+b=0", "3. Quadratic: ax^2+bx+c=0"};
        tft.setTextSize(2);
        for (int i = 0; i < 3; i++) {
            int y = 40 + i * 40;
            if (i == solverSel) {
                tft.fillRect(2, y-2, SW-4, 32, C_SEL);
                tft.setTextColor(C_FG);
            } else {
                tft.setTextColor(C_DIM);
            }
            tft.setCursor(10, y); tft.print(types[i]);
        }
    } else if (solverPhase == 1) {
        if (solverType == 0) {
            // Equation input: LHS = RHS
            tft.setTextSize(2); tft.setTextColor(C_DIM);
            tft.setCursor(6, 30); tft.print("Enter equation:");

            tft.setTextSize(2);
            int y = 70;
            // LHS
            tft.setTextColor(!solverEditingRHS ? C_CYAN : C_FG);
            tft.setCursor(6, y); tft.print(solverLHS);
            if (!solverEditingRHS) tft.print("_");

            tft.setTextColor(C_DIM);
            tft.setCursor(6 + max((int)solverLHS.length(), 1)*12 + 12, y);
            tft.print("=");

            // RHS
            tft.setTextColor(solverEditingRHS ? C_CYAN : C_FG);
            tft.setCursor(6 + max((int)solverLHS.length(), 1)*12 + 24, y);
            tft.print(solverRHS);
            if (solverEditingRHS) tft.print("_");

            tft.setTextSize(1); tft.setTextColor(C_DIM);
            tft.setCursor(6, SH-12);
            tft.print("Enter='=' or solve  ESC=Back");
        } else {
            // Coefficient input
            const char* labels1[] = {"a", "b"};
            const char* labels2[] = {"a", "b", "c"};
            const char** labels = (solverType == 1) ? labels1 : labels2;
            int maxCoef = (solverType == 1) ? 2 : 3;

            tft.setTextSize(2); tft.setTextColor(C_DIM);
            tft.setCursor(6, 30);
            tft.print(solverType == 1 ? "ax + b = 0" : "ax^2 + bx + c = 0");

            tft.setTextSize(2);
            double* coefs[] = {&sA, &sB, &sC};
            for (int i = 0; i < maxCoef; i++) {
                int y = 70 + i * 36;
                tft.setTextColor(i == solverCoefStep ? C_CYAN : C_DIM);
                tft.setCursor(10, y);
                tft.print(labels[i]); tft.print(" = ");
                tft.setTextColor(C_FG);
                if (i == solverCoefStep) {
                    tft.print(solverCoefBuf); tft.print("_");
                } else if (i < solverCoefStep) {
                    tft.print(fmtNum(*coefs[i]));
                }
            }
        }
    } else if (solverPhase == 2) {
        // Result
        tft.setTextSize(2); tft.setTextColor(C_DIM);
        tft.setCursor(6, 30); tft.print("Solution:");

        tft.setTextSize(2); tft.setTextColor(C_CYAN);
        // Word-wrap result if needed
        tft.setCursor(6, 70);
        tft.print(solverResult);

        tft.setTextSize(1); tft.setTextColor(C_DIM);
        tft.setCursor(6, SH-12); tft.print("Enter=New  ESC=Menu");
    }
}

void solveLinQuad() {
    if (solverType == 1) {
        if (fabs(sA) < 1e-15) {
            solverResult = (fabs(sB) < 1e-15) ? "Infinite solutions" : "No solution";
        } else {
            solverResult = "x = " + fmtNum(-sB / sA);
        }
    } else {
        if (fabs(sA) < 1e-15) {
            if (fabs(sB) < 1e-15) solverResult = (fabs(sC)<1e-15) ? "Infinite" : "No solution";
            else solverResult = "x = " + fmtNum(-sC/sB);
        } else {
            double disc = sB*sB - 4*sA*sC;
            if (disc > 1e-12) {
                double x1 = (-sB + sqrt(disc)) / (2*sA);
                double x2 = (-sB - sqrt(disc)) / (2*sA);
                solverResult = "x=" + fmtNum(x1) + ", " + fmtNum(x2);
            } else if (fabs(disc) <= 1e-12) {
                solverResult = "x = " + fmtNum(-sB/(2*sA));
            } else {
                double re = -sB/(2*sA);
                double im = sqrt(-disc)/(2*fabs(sA));
                solverResult = fmtNum(re) + " +/- " + fmtNum(im) + "i";
            }
        }
    }
    solverPhase = 2;
}

// ============ CALCULUS MODE ============

int calcSel = 0;    // sub-menu selection
int calcPhase = 0;  // 0=menu, 1=input, 2=result
String calcInput1;   // f(x)
String calcInput2;   // x-value or lower bound
String calcInput3;   // upper bound
int calcInputStep = 0;
String calcResult;

void drawCalculus() {
    tft.fillScreen(C_BG);
    drawStatusBar("CALCULUS");

    if (calcPhase == 0) {
        const char* items[] = {
            "1. Derivative (symbolic)",
            "2. Derivative (numeric)",
            "3. Integral (definite)",
            "4. Integral (indefinite)"
        };
        tft.setTextSize(2);
        for (int i = 0; i < 4; i++) {
            int y = 30 + i * 36;
            if (i == calcSel) {
                tft.fillRect(2, y-2, SW-4, 30, C_SEL);
                tft.setTextColor(C_FG);
            } else {
                tft.setTextColor(C_DIM);
            }
            tft.setCursor(10, y); tft.print(items[i]);
        }
    } else if (calcPhase == 1) {
        tft.setTextSize(2); tft.setTextColor(C_DIM);
        tft.setCursor(6, 24);

        const char* prompts[][3] = {
            {"f(x)=", "", ""},
            {"f(x)=", "x=", ""},
            {"f(x)=", "a=", "b="},
            {"f(x)=", "", ""}
        };

        int nInputs[] = {1, 2, 3, 1};
        String* inputs[] = {&calcInput1, &calcInput2, &calcInput3};

        tft.print(calcSel == 0 ? "Symbolic d/dx" :
                  calcSel == 1 ? "Numeric d/dx" :
                  calcSel == 2 ? "Definite integral" : "Indefinite integral");

        for (int i = 0; i < nInputs[calcSel]; i++) {
            int y = 56 + i * 36;
            tft.setTextColor(i == calcInputStep ? C_CYAN : C_DIM);
            tft.setCursor(10, y);
            tft.print(prompts[calcSel][i]);
            tft.setTextColor(C_FG);
            if (i == calcInputStep) {
                tft.print(*inputs[i]); tft.print("_");
            } else if (i < calcInputStep) {
                tft.print(*inputs[i]);
            }
        }
    } else if (calcPhase == 2) {
        tft.setTextSize(2); tft.setTextColor(C_DIM);
        tft.setCursor(6, 30); tft.print("Result:");

        tft.setTextColor(C_CYAN);
        tft.setCursor(6, 70);
        // Handle long result by wrapping
        int maxCharsPerLine = SW / 12 - 1;
        String r = calcResult;
        int y = 70;
        while (r.length() > 0 && y < SH - 20) {
            String line = r.substring(0, min((int)r.length(), maxCharsPerLine));
            tft.setCursor(6, y);
            tft.print(line);
            r = r.substring(line.length());
            y += 20;
        }

        tft.setTextSize(1); tft.setTextColor(C_DIM);
        tft.setCursor(6, SH-12); tft.print("Enter=New ESC=Menu");
    }
}

void doCalculusCompute() {
    switch (calcSel) {
        case 0: // symbolic derivative
            calcResult = "f'(x)=" + symbolicDerivative(calcInput1);
            break;
        case 1: { // numeric derivative
            double x0 = atof(calcInput2.c_str());
            double d = numericDerivative(calcInput1, x0);
            calcResult = isnan(d) ? "Error" : ("f'(" + fmtNum(x0) + ")=" + fmtNum(d));
            break;
        }
        case 2: { // definite integral
            double a = atof(calcInput2.c_str());
            double b = atof(calcInput3.c_str());
            double val = simpsonIntegral(calcInput1, a, b);
            calcResult = isnan(val) ? "Error" : ("= " + fmtNum(val));
            break;
        }
        case 3: // indefinite integral
            calcResult = "F(x)=" + symbolicIntegral(calcInput1);
            break;
    }
    calcPhase = 2;
}

// ============ MENU ============

constexpr int N_MENU = 6;
const char* menuLabels[] = {"Calculate", "Y=", "Graph", "Table", "Solver", "Calculus"};

void drawMenu() {
    tft.fillScreen(C_BG);
    drawStatusBar("MENU");

    tft.setTextSize(2);
    for (int i = 0; i < N_MENU; i++) {
        int y = 26 + i * 32;
        if (i == menuSel) {
            tft.fillRect(2, y-2, SW-4, 28, C_SEL);
            tft.setTextColor(C_FG);
        } else {
            tft.setTextColor(C_DIM);
        }
        tft.setCursor(14, y);
        tft.print(i+1); tft.print(". "); tft.print(menuLabels[i]);
    }
}

void menuSelect(int item) {
    switch (item) {
        case 0: mode = M_CALC; break;
        case 1: mode = M_YEDIT; yEditing = false; break;
        case 2: mode = M_GRAPH; traceOn = false; break;
        case 3: mode = M_TABLE; tblScroll = 0; break;
        case 4: mode = M_SOLVER; solverPhase = 0; solverSel = 0; break;
        case 5: mode = M_CALCULUS; calcPhase = 0; calcSel = 0; break;
    }
    dirty = true;
}

// ============ INPUT HANDLING ============

void handleKey(char k) {
    dirty = true;

    switch (mode) {

    case M_CALC:
        if (k == KEY_ESC) { mode = M_MENU; menuSel = 0; }
        else if (k == KEY_BS) { if (inp.length() > 0) inp.remove(inp.length()-1); }
        else if (k == KEY_ENTER) { doCalcEval(); }
        else if (k >= 0x20 && k <= 0x7E && inp.length() < 60) { inp += k; }
        break;

    case M_MENU:
        if (k == KEY_ESC) { mode = M_CALC; }
        else if (k == (char)KEY_UP) { menuSel = (menuSel - 1 + N_MENU) % N_MENU; }
        else if (k == (char)KEY_DOWN) { menuSel = (menuSel + 1) % N_MENU; }
        else if (k == KEY_ENTER) { menuSelect(menuSel); }
        else if (k >= '1' && k <= '6') { menuSelect(k - '1'); }
        break;

    case M_YEDIT:
        if (k == KEY_ESC) {
            if (yEditing) { yExprs[yEditSel] = yEditBuf; yEditing = false; }
            else { mode = M_MENU; menuSel = 1; }
        } else if (!yEditing) {
            if (k == (char)KEY_UP) { yEditSel = (yEditSel - 1 + 4) % 4; }
            else if (k == (char)KEY_DOWN) { yEditSel = (yEditSel + 1) % 4; }
            else if (k == KEY_ENTER) { yEditing = true; yEditBuf = yExprs[yEditSel]; }
        } else {
            if (k == KEY_ENTER) { yExprs[yEditSel] = yEditBuf; yEditing = false; }
            else if (k == KEY_BS) { if (yEditBuf.length() > 0) yEditBuf.remove(yEditBuf.length()-1); }
            else if (k >= 0x20 && k <= 0x7E && yEditBuf.length() < 40) { yEditBuf += k; }
        }
        break;

    case M_GRAPH:
        if (k == KEY_ESC) { mode = M_MENU; menuSel = 2; }
        else if (k == 'w' || k == 'W') {
            mode = M_GRAPH_WIN; winField = 0;
            winBuf = fmtNum(gXmin);
        }
        else if (k == 'z' || k == 'Z') {
            // Zoom in (halve range)
            float xc = (gXmin+gXmax)/2, yc = (gYmin+gYmax)/2;
            float xr = (gXmax-gXmin)/4, yr = (gYmax-gYmin)/4;
            gXmin = xc-xr; gXmax = xc+xr; gYmin = yc-yr; gYmax = yc+yr;
        }
        else if (k == 'o' || k == 'O') {
            // Zoom out (double range)
            float xc = (gXmin+gXmax)/2, yc = (gYmin+gYmax)/2;
            float xr = (gXmax-gXmin), yr = (gYmax-gYmin);
            gXmin = xc-xr; gXmax = xc+xr; gYmin = yc-yr; gYmax = yc+yr;
        }
        else if (k == 't' || k == 'T') {
            traceOn = !traceOn;
            traceX = (gXmin + gXmax) / 2;
            // Find first active function
            for (int i = 0; i < 4; i++) {
                if (yActive[i] && yExprs[i].length() > 0) { traceFn = i; break; }
            }
        }
        else if (traceOn) {
            float step = (gXmax - gXmin) / SW;
            if (k == (char)KEY_LEFT) traceX -= step * 3;
            else if (k == (char)KEY_RIGHT) traceX += step * 3;
            else if (k == (char)KEY_UP) {
                // Next active function
                for (int i = 1; i <= 4; i++) {
                    int fn = (traceFn + i) % 4;
                    if (yActive[fn] && yExprs[fn].length() > 0) { traceFn = fn; break; }
                }
            }
            else if (k == (char)KEY_DOWN) {
                for (int i = 1; i <= 4; i++) {
                    int fn = (traceFn - i + 4) % 4;
                    if (yActive[fn] && yExprs[fn].length() > 0) { traceFn = fn; break; }
                }
            }
        }
        break;

    case M_GRAPH_WIN: {
        float* fields[] = {&gXmin, &gXmax, &gYmin, &gYmax};
        if (k == KEY_ESC) { mode = M_GRAPH; }
        else if (k == KEY_BS) { if (winBuf.length() > 0) winBuf.remove(winBuf.length()-1); }
        else if (k == KEY_ENTER) {
            *fields[winField] = atof(winBuf.c_str());
            winField++;
            if (winField >= 4) { mode = M_GRAPH; }
            else {
                winBuf = fmtNum(*fields[winField]);
            }
        }
        else if ((k >= '0' && k <= '9') || k == '.' || k == '-') {
            if (winBuf.length() < 15) winBuf += k;
        }
        break;
    }

    case M_TABLE:
        if (k == KEY_ESC) { mode = M_MENU; menuSel = 3; }
        else if (k == (char)KEY_UP) { tblScroll--; }
        else if (k == (char)KEY_DOWN) { tblScroll++; }
        else if (k == 's' || k == 'S') {
            mode = M_TABLE_SET; tblSetField = 0;
            tblSetBuf = fmtNum(tblStart);
        }
        break;

    case M_TABLE_SET: {
        float* fields[] = {&tblStart, &tblStep};
        if (k == KEY_ESC) { mode = M_TABLE; }
        else if (k == KEY_BS) { if (tblSetBuf.length() > 0) tblSetBuf.remove(tblSetBuf.length()-1); }
        else if (k == KEY_ENTER) {
            *fields[tblSetField] = atof(tblSetBuf.c_str());
            tblSetField++;
            if (tblSetField >= 2) { mode = M_TABLE; tblScroll = 0; }
            else { tblSetBuf = fmtNum(*fields[tblSetField]); }
        }
        else if ((k >= '0' && k <= '9') || k == '.' || k == '-') {
            if (tblSetBuf.length() < 15) tblSetBuf += k;
        }
        break;
    }

    case M_SOLVER:
        if (k == KEY_ESC) {
            if (solverPhase == 0) { mode = M_MENU; menuSel = 4; }
            else { solverPhase = 0; solverSel = 0; }
        }
        else if (solverPhase == 0) {
            if (k == (char)KEY_UP) solverSel = (solverSel - 1 + 3) % 3;
            else if (k == (char)KEY_DOWN) solverSel = (solverSel + 1) % 3;
            else if (k == KEY_ENTER || (k >= '1' && k <= '3')) {
                int sel = (k >= '1') ? (k - '1') : solverSel;
                solverType = sel;
                solverPhase = 1;
                if (sel == 0) { solverLHS = ""; solverRHS = ""; solverEditingRHS = false; }
                else { solverCoefStep = 0; solverCoefBuf = ""; sA = sB = sC = 0; }
            }
        }
        else if (solverPhase == 2) {
            if (k == KEY_ENTER) { solverPhase = 0; solverSel = 0; }
        }
        break;

    case M_SOLVER_EQ:
        // handled below in M_SOLVER phase 1
        break;

    case M_CALCULUS:
        if (k == KEY_ESC) {
            if (calcPhase == 0) { mode = M_MENU; menuSel = 5; }
            else { calcPhase = 0; }
        }
        else if (calcPhase == 0) {
            if (k == (char)KEY_UP) calcSel = (calcSel - 1 + 4) % 4;
            else if (k == (char)KEY_DOWN) calcSel = (calcSel + 1) % 4;
            else if (k == KEY_ENTER || (k >= '1' && k <= '4')) {
                int sel = (k >= '1') ? (k - '1') : calcSel;
                calcSel = sel;
                calcPhase = 1;
                calcInputStep = 0;
                calcInput1 = ""; calcInput2 = ""; calcInput3 = "";
            }
        }
        else if (calcPhase == 2) {
            if (k == KEY_ENTER) { calcPhase = 0; }
        }
        break;

    default: break;
    }

    // Handle solver phase 1 input (within M_SOLVER mode)
    if (mode == M_SOLVER && solverPhase == 1) {
        if (solverType == 0) {
            // Equation solver input
            if (k == KEY_ESC) { solverPhase = 0; }
            else if (k == KEY_BS) {
                String& buf = solverEditingRHS ? solverRHS : solverLHS;
                if (buf.length() > 0) buf.remove(buf.length()-1);
            }
            else if (k == '=' && !solverEditingRHS) {
                solverEditingRHS = true;
            }
            else if (k == KEY_ENTER && solverEditingRHS && solverRHS.length() > 0) {
                double x = solveEquation(solverLHS, solverRHS);
                solverResult = isnan(x) ? "No solution found" : ("x = " + fmtNum(x));
                solverPhase = 2;
            }
            else if (k >= 0x20 && k <= 0x7E && k != '=') {
                String& buf = solverEditingRHS ? solverRHS : solverLHS;
                if (buf.length() < 30) buf += k;
            }
        } else {
            // Coefficient input for linear/quadratic
            if (k == KEY_ESC) { solverPhase = 0; }
            else if (k == KEY_BS) {
                if (solverCoefBuf.length() > 0) solverCoefBuf.remove(solverCoefBuf.length()-1);
            }
            else if (k == KEY_ENTER && solverCoefBuf.length() > 0) {
                double val = atof(solverCoefBuf.c_str());
                double* coefs[] = {&sA, &sB, &sC};
                *coefs[solverCoefStep] = val;
                solverCoefStep++;
                int maxCoef = (solverType == 1) ? 2 : 3;
                if (solverCoefStep >= maxCoef) {
                    solveLinQuad();
                } else {
                    solverCoefBuf = "";
                }
            }
            else if ((k >= '0' && k <= '9') || k == '.' || k == '-') {
                if (solverCoefBuf.length() < 15) solverCoefBuf += k;
            }
        }
    }

    // Handle calculus phase 1 input
    if (mode == M_CALCULUS && calcPhase == 1) {
        if (k == KEY_ESC) { calcPhase = 0; }
        else if (k == KEY_BS) {
            String* inputs[] = {&calcInput1, &calcInput2, &calcInput3};
            String& buf = *inputs[calcInputStep];
            if (buf.length() > 0) buf.remove(buf.length()-1);
        }
        else if (k == KEY_ENTER) {
            int nInputs[] = {1, 2, 3, 1};
            calcInputStep++;
            if (calcInputStep >= nInputs[calcSel]) {
                doCalculusCompute();
            }
        }
        else if (k >= 0x20 && k <= 0x7E) {
            String* inputs[] = {&calcInput1, &calcInput2, &calcInput3};
            String& buf = *inputs[calcInputStep];
            if (buf.length() < 40) buf += k;
        }
    }
}

// ============ MAIN ============

void setup() {
    Serial.begin(115200);
    delay(300);

    pinMode(TFT_BLK, OUTPUT);
    digitalWrite(TFT_BLK, HIGH);

    SPI.begin(TFT_CLK, -1, TFT_MOSI, TFT_CS);
    tft.begin();
    tft.setSPISpeed(40000000);
    tft.setRotation(1);
    tft.fillScreen(C_BG);

    kb.begin();

    dirty = true;
    Serial.println("Nash Calculator v2.0 — TI-84 Style");
}

void loop() {
    char k = kb.read();
    if (k) {
        Serial.printf("Key: 0x%02X\n", k);
        handleKey(k);
    }

    if (dirty) {
        switch (mode) {
            case M_CALC:      drawCalc(); break;
            case M_MENU:      drawMenu(); break;
            case M_YEDIT:     drawYEdit(); break;
            case M_GRAPH:     drawGraph(); break;
            case M_GRAPH_WIN: drawGraphWindow(); break;
            case M_TABLE:     drawTable(); break;
            case M_TABLE_SET: drawTableSettings(); break;
            case M_SOLVER:    drawSolver(); break;
            case M_CALCULUS:  drawCalculus(); break;
            default: break;
        }
        dirty = false;
    }

    delay(10);
}
