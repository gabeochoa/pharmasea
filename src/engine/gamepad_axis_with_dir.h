
#pragma once

#include "../vendor_include.h"

struct GamepadAxisWithDir {
    raylib::GamepadAxis axis;
    float dir = -1;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value4b(axis);
        s.value4b(dir);
    }
};
