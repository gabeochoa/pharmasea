
#pragma once
#include "external_include.h"
#include "raylib.h"

struct GamepadAxisWithDir {
    GamepadAxis axis;
    float dir = -1;
};
