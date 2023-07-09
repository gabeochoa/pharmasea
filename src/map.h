

#pragma once

#include "engine.h"
#include "engine/container_cast.h"
#include "engine/log.h"
#include "external_include.h"
//
#include "entityhelper.h"
#include "level_info.h"

struct Map {
    LobbyMapInfo lobby_info;
    GameMapInfo game_info;

    Entities remote_players_NOT_SERIALIZED;
    std::string seed;

    // This gets called on every network frame because
    // the serializer uses the default contructor
    Map() {}

    Map(const std::string& _seed) : seed(_seed) {
        // TODO this is needed for the items to be regenerated
        update_seed(seed);
    }

    void update_seed(const std::string& s) { game_info.update_seed(s); }

    Items items() const {
        return in_lobby_state() ? lobby_info.items : game_info.items;
    }

    // TODO should be const?
    Entities entities() {
        return in_lobby_state() ? lobby_info.entities : game_info.entities;
    }

    void onUpdate(float dt) { _onUpdate(remote_players_NOT_SERIALIZED, dt); }

    void _onUpdate(Entities players, float dt) {
        TRACY_ZONE_SCOPED;
        // TODO add to debug overlay
        // log_info("num items {}", items().size());

        if (in_lobby_state()) {
            lobby_info.ensure_generated_map(seed);
            lobby_info.onUpdate(players, dt);
        } else {
            game_info.ensure_generated_map(seed);
            game_info.onUpdate(players, dt);
        }
    }

    // TODO make const again
    void onDraw(float dt) {
        TRACY_ZONE_SCOPED;
        // TODO merge this into normal render pipeline
        SystemManager::get().render_entities(remote_players_NOT_SERIALIZED, dt);

        if (in_lobby_state()) {
            lobby_info.onDraw(dt);
        } else {
            game_info.onDraw(dt);
        }
    }

    void onDrawUI(float dt) {
        TRACY_ZONE_SCOPED;

        // TODO merge this into normal render pipeline
        SystemManager::get().render_ui(remote_players_NOT_SERIALIZED, dt);
        if (in_lobby_state()) {
            lobby_info.onDrawUI(dt);
        } else {
            game_info.onDrawUI(dt);
        }
    }

    [[nodiscard]] bool in_lobby_state() const {
        return GameState::get().is(game::State::Lobby) ||
               GameState::get().is_paused_in(game::State::Lobby);
    }

    // These are called before every "send_map_state" when server
    // sends everything over to clients
    void grab_things() {
        if (in_lobby_state()) {
            lobby_info.grab_things();
        } else {
            game_info.grab_things();
        }
    }
    // TODO do we need this
    void ensure_generated_map() {
        // game_info.ensure_generated_map(seed);
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.object(lobby_info);
        s.object(game_info);
    }
};
