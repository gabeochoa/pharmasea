
#pragma once

#include "../vendor_include.h"
#include "raylib.h"

struct GamepadAxisWithDir {
    GamepadAxis axis;
    float dir = -1;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value4b(axis);
        s.value4b(dir);
    }
};
