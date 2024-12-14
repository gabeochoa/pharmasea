
#pragma once

#include "../vendor_include.h"

struct GamepadAxisWithDir {
    raylib::GamepadAxis axis;
    float dir = -1;

   private:
    template<class Archive>
    void serialize(Archive& archive) {
        archive(axis, dir);
    }
};
