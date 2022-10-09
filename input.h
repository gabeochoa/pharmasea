
#pragma once

#include "external_include.h"

struct Input {
    std::map<int, bool> pressedSinceLast;

    void onUpdate(float) {
        int key = GetKeyPressed();
        if (key != 0) std::cout << "keypressed: " << key << std::endl;
        pressedSinceLast[key] = true;
    }

    bool pressed(int key) { return pressedSinceLast[key]; }

    bool eat(int key) {
        bool pressed = pressedSinceLast[key];
        pressedSinceLast[key] = false;
        return pressed;
    }

    // TODO migrate to globals
} input;
