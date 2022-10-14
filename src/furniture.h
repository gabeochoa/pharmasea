
#pragma once 

#include "external_include.h"
#include "entity.h"

struct Furniture : public Entity {

    Furniture(vec2 pos, Color face_color_in, Color base_color_in): Entity(pos, face_color_in, base_color_in) {}

    virtual bool can_rotate() {
        return false;
    }

    virtual bool can_be_picked_up() {
        return false;
    }
};
