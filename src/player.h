
#pragma once

#include "base_player.h"
#include "components/can_be_ghost_player.h"
#include "components/can_hold_furniture.h"
#include "components/collects_user_input.h"
#include "engine/keymap.h"
#include "globals.h"
#include "raylib.h"
#include "statemanager.h"
//
#include "furniture.h"

struct Player : public BasePlayer {
    // Theres no players not in game menu state,
    const menu::State state = menu::State::Game;

    std::string username;

    void add_static_components() {
        addComponent<CanBeGhostPlayer>();
        addComponent<CollectsUserInput>();
        addComponent<RespondsToUserInput>();
    }

    // NOTE: this is kept public because we use it in the network when prepping
    // server players
    Player() : BasePlayer({0, 0, 0}, WHITE, WHITE) { add_static_components(); }

    Player(vec3 p, Color face_color_in, Color base_color_in)
        : BasePlayer(p, face_color_in, base_color_in) {
        add_static_components();
    }
    Player(vec2 p, Color face_color_in, Color base_color_in)
        : BasePlayer(p, face_color_in, base_color_in) {
        add_static_components();
    }
    Player(vec2 location)
        : BasePlayer({location.x, 0, location.y}, {0, 255, 0, 255},
                     {255, 0, 0, 255}) {
        add_static_components();
    }

    virtual bool is_collidable() override {
        return get<CanBeGhostPlayer>().is_not_ghost();
    }
};
