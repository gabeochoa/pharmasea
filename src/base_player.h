

#pragma once

#include "components/can_highlight_others.h"
#include "raylib.h"
//
#include "engine/globals_register.h"
#include "engine/keymap.h"
#include "person.h"
//
#include "furniture.h"

// TODO add more information on what the difference is between Person and
// BasePlayer and Player
struct BasePlayer : public Person {
    float player_reach = 1.25f;
    std::shared_ptr<Furniture> held_furniture;

    void add_static_components() { addComponent<CanHighlightOthers>(); }

    BasePlayer(vec3 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {
        add_static_components();
    }
    BasePlayer(vec2 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {
        add_static_components();
    }
    BasePlayer() : Person({0, 0, 0}, ui::color::white, ui::color::white) {
        add_static_components();
    }
    BasePlayer(vec2 location)
        : Person({location.x, 0, location.y}, {0, 255, 0, 255},
                 {255, 0, 0, 255}) {
        add_static_components();
    }

    virtual float base_speed() override { return 7.5f; }

    virtual vec3 update_xaxis_position(float dt) override = 0;
    virtual vec3 update_zaxis_position(float dt) override = 0;

    void highlight_facing_furniture() {}

    virtual void planning_update(float dt) override {
        Person::planning_update(dt);

        // TODO if cannot be placed in this spot make it obvious to the user
        if (held_furniture != nullptr) {
            auto new_pos = this->get<Transform>().position;
            if (this->get<Transform>().face_direction &
                Transform::FrontFaceDirection::FORWARD) {
                new_pos.z += TILESIZE;
            }
            if (this->get<Transform>().face_direction &
                Transform::FrontFaceDirection::RIGHT) {
                new_pos.x += TILESIZE;
            }
            if (this->get<Transform>().face_direction &
                Transform::FrontFaceDirection::BACK) {
                new_pos.z -= TILESIZE;
            }
            if (this->get<Transform>().face_direction &
                Transform::FrontFaceDirection::LEFT) {
                new_pos.x -= TILESIZE;
            }

            held_furniture->update_position(new_pos);
        }
    }
};
