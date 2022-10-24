
#pragma once

#include "entity.h"
#include "external_include.h"

struct Furniture : public Entity {
    Furniture(raylib::vec2 pos, raylib::Color face_color_in)
        : Entity(pos, face_color_in, face_color_in) {}
    Furniture(raylib::vec3 pos, raylib::Color face_color_in)
        : Entity(pos, face_color_in, face_color_in) {}
    Furniture(raylib::vec2 pos, raylib::Color face_color_in, raylib::Color base_color_in)
        : Entity(pos, face_color_in, base_color_in) {}

    virtual void update_held_item_position() override {
        if (held_item != nullptr) {
            auto new_pos = this->position;
            new_pos.y += TILESIZE / 4;
            held_item->update_position(new_pos);
        }
    }

    virtual bool can_rotate() { return false; }

    virtual bool add_to_navmesh() override { return true; }

    virtual bool can_be_picked_up() { return false; }
    virtual bool can_place_item_into() override { return false; }
};
