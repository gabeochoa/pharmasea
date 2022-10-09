
#pragma once

#include "external_include.h"

struct Input {
    std::deque<int> pressedSinceLast;

    void onUpdate(float) {
        pressedSinceLast.clear();
        while (true) {
            int key = GetKeyPressed();
            if (key != 0)
                std::cout << "keypressed: " << key << std::endl;
            else {
                break;
            }
            pressedSinceLast.push_back(key);
        }
    }

    // TODO migrate to globals
} input;
