
#pragma once

#include "event.h"
#include "external_include.h"
#include "singleton.h"

SINGLETON_FWD(Input)
struct Input {
    SINGLETON(Input)

    std::deque<int> pressedSinceLast;

    void onUpdate(float) {
        pressedSinceLast.clear();
        while (true) {
            int key = GetKeyPressed();
            if (key != 0) {
                // std::cout << "keypressed: " << key << std::endl;
            } else {
                break;
            }
            pressedSinceLast.push_back(key);
        }
    }
};
