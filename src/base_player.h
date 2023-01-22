

#pragma once

#include "components/can_be_ghost_player.h"
#include "components/can_highlight_others.h"
#include "components/can_hold_furniture.h"
#include "components/responds_to_user_input.h"
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
    void add_static_components() {
        addComponent<CanHighlightOthers>();
        addComponent<CanHoldFurniture>();
    }

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
};
