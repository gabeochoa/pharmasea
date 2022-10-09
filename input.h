
#pragma once

#include "external_include.h"
#include "event.h"

struct Input;
static std::shared_ptr<Input> input;
struct Input {
    std::deque<int> pressedSinceLast;

    void onUpdate(float) {
        pressedSinceLast.clear();
        while (true) {
            int key = GetKeyPressed();
            if (key != 0){
                // std::cout << "keypressed: " << key << std::endl;
            }
            else {
                break;
            }
            pressedSinceLast.push_back(key);
        }
    }

    inline static Input* create() { return new Input(); }
    inline static Input& get() {
        if (!input) input.reset(Input::create());
        return *input;
    }

};

