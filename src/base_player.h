

#pragma once

#include "components/can_be_ghost_player.h"
#include "components/can_highlight_others.h"
#include "components/can_hold_furniture.h"
#include "components/collects_user_input.h"
#include "components/responds_to_user_input.h"
#include "engine/globals_register.h"
#include "engine/keymap.h"
#include "furniture.h"
#include "globals.h"
#include "person.h"
#include "raylib.h"
#include "statemanager.h"

// TODO add more information on what the difference is between Person and
// BasePlayer and Player
struct BasePlayer : public Person {
    void add_static_components() {
        addComponent<CanHighlightOthers>();
        addComponent<CanHoldFurniture>();

        // addComponent<HasBaseSpeed>().update(10.f);
        get<HasBaseSpeed>().update(7.5f);
    }

    BasePlayer(vec3 p) : Person(p) { add_static_components(); }
    BasePlayer() : BasePlayer({0, 0, 0}) {}
    BasePlayer(vec2 location) : BasePlayer({location.x, 0, location.y}) {}
};

struct RemotePlayer : public BasePlayer {
    // TODO what is a reasonable default value here?
    // TODO who sets this value?
    int client_id = -1;

    RemotePlayer(vec3 p) : BasePlayer(p) {}
    RemotePlayer() : BasePlayer({0, 0, 0}) {}
    RemotePlayer(vec2 location) : BasePlayer({location.x, 0, location.y}) {}

    static void update_remotely(std::shared_ptr<Entity> entity, float* location,
                                std::string username, int facing_direction) {
        HasName& hasname = entity->get<HasName>();
        hasname.name = username;

        entity->get<HasName>().name = username;
        Transform& transform = entity->get<Transform>();
        // TODO add setters
        transform.position = vec3{location[0], location[1], location[2]};
        transform.face_direction =
            static_cast<Transform::FrontFaceDirection>(facing_direction);
    }
};

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
    Player() : BasePlayer({0, 0, 0}) { add_static_components(); }

    Player(vec3 p) : BasePlayer(p) { add_static_components(); }
    Player(vec2 location) : Player({location.x, 0, location.y}) {}
};
