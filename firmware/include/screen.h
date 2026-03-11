// screen.h — Abstract screen base + stack-based screen manager
#pragma once
#include <Arduino.h>
#include "display.h"

// Base class for all screens
class Screen {
public:
    virtual ~Screen() {}
    virtual void draw(TFT_eSprite& fb) = 0;
    virtual void handleInput(char key) = 0;
    virtual const char* title() { return ""; }
};

// Stack-based screen manager (max 8 deep)
class ScreenManager {
public:
    void push(Screen* s);
    void pop();
    Screen* current();
    int depth() { return _top + 1; }

private:
    static constexpr int MAX_SCREENS = 8;
    Screen* _stack[MAX_SCREENS] = {};
    int     _top = -1;
};
