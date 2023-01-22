

#pragma once

#include "components/collects_user_input.h"
#include "player.h"

struct LocalPlayer : public Player {
    void add_static_components() { addComponent<CollectsUserInput>(); }

    LocalPlayer() : Player() { add_static_components(); }
    LocalPlayer(vec2 pos) : Player(pos) { add_static_components(); }
};
