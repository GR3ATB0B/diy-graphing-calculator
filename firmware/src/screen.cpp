// screen.cpp — Screen manager implementation
#include "screen.h"

void ScreenManager::push(Screen* s) {
    if (_top < MAX_SCREENS - 1) {
        _stack[++_top] = s;
    }
}

void ScreenManager::pop() {
    if (_top >= 0) {
        _top--;
    }
}

Screen* ScreenManager::current() {
    return (_top >= 0) ? _stack[_top] : nullptr;
}
