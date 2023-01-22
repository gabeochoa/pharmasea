

#pragma once

#include "components/can_be_ghost_player.h"
#include "components/can_highlight_others.h"
#include "components/can_hold_furniture.h"
#include "components/responds_to_user_input.h"
#include "raylib.h"
//
#include "engine/globals_register.h"
#include "engine/keymap.h"
#include "entity.h"
//
#include "furniture.h"

// TODO add more information on what the difference is between Person and
// BasePlayer and Player
struct BasePlayer : public Person {
    void add_static_components() {
        this->addComponent<Transform>();
        this->addComponent<CanHighlightOthers>();
        this->addComponent<CanHoldFurniture>();
        this->addComponent<HasBaseSpeed>().update(7.5f);
    }

    BasePlayer(vec3 p, Color, Color) : Person(p) { add_static_components(); }
    BasePlayer() : Person({0, 0, 0}) { add_static_components(); }
};
