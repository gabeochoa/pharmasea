
#pragma once 

#include "../external_include.h"
#include "../entity.h"
#include "../globals.h"

struct CanBePickedUp {};

struct Register : public Entity, public CanBePickedUp {

    Register(vec2 pos): Entity(pos, BLACK, DARKGRAY) {}

};
