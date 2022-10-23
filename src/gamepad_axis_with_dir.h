
#pragma once
#include "external_include.h"

struct GamepadAxisWithDir {
    raylib::GamepadAxis axis;
    float dir = -1;
};
