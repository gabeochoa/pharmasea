
#pragma once

#include "entity.h"
#include "external_include.h"

struct Furniture : public Entity {
    Furniture(vec2 pos, Color face_color_in)
        : Entity(pos, face_color_in, face_color_in) {}
    Furniture(vec3 pos, Color face_color_in)
        : Entity(pos, face_color_in, face_color_in) {}
    Furniture(vec2 pos, Color face_color_in, Color base_color_in)
        : Entity(pos, face_color_in, base_color_in) {}

    virtual bool can_rotate() { return false; }

    virtual bool add_to_navmesh() override { return true; }

    virtual bool can_be_picked_up() { return false; }
    virtual bool can_place_item_into() override { return false; }
};
