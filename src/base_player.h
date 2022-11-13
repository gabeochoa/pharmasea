

#pragma once

#include "globals_register.h"
#include "keymap.h"
#include "person.h"
#include "raylib.h"
//
#include "furniture.h"

struct BasePlayer : public Person {
    float player_reach = 1.25f;
    std::shared_ptr<Furniture> held_furniture;

    BasePlayer(vec3 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    BasePlayer(vec2 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    BasePlayer() : Person({0, 0, 0}, {0, 255, 0, 255}, {255, 0, 0, 255}) {}
    BasePlayer(vec2 location)
        : Person({location.x, 0, location.y}, {0, 255, 0, 255},
                 {255, 0, 0, 255}) {}

    virtual float base_speed() override { return 7.5f; }

    virtual vec3 update_xaxis_position(float dt) override = 0;
    virtual vec3 update_zaxis_position(float dt) override = 0;

    void highlight_facing_furniture() {
        auto match = EntityHelper::getMatchingEntityInFront<Furniture>(
            vec::to2(this->position), player_reach, this->face_direction,
            [](std::shared_ptr<Furniture>) { return true; });
        if (match) {
            match->is_highlighted = true;
        }
    }

    virtual void update(float dt) override {
        Person::update(dt);
        highlight_facing_furniture();

        // TODO if cannot be placed in this spot
        // make it obvious to the user
        if (held_furniture != nullptr) {
            auto new_pos = this->position;
            if (this->face_direction & FrontFaceDirection::FORWARD) {
                new_pos.z += TILESIZE;
            }
            if (this->face_direction & FrontFaceDirection::RIGHT) {
                new_pos.x += TILESIZE;
            }
            if (this->face_direction & FrontFaceDirection::BACK) {
                new_pos.z -= TILESIZE;
            }
            if (this->face_direction & FrontFaceDirection::LEFT) {
                new_pos.x -= TILESIZE;
            }

            held_furniture->update_position(new_pos);
        }
    }
};
