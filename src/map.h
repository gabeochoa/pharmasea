

#pragma once

#include "engine.h"
#include "engine/log.h"
#include "external_include.h"
//
#include "entityhelper.h"
#include "level_info.h"
#include "remote_player.h"
#include "statemanager.h"

struct Map {
    LobbyMapInfo lobby_info;
    GameMapInfo game_info;

    std::vector<std::shared_ptr<RemotePlayer>> remote_players_NOT_SERIALIZED;
    std::string seed;

    Map(const std::string& _seed = "default_seed") : seed(_seed) {
        // TODO this is needed for the items to be regenerated
        update_seed(seed);
    }

    void update_seed(const std::string& s) { game_info.update_seed(s); }

    Items items() const {
        return in_lobby_state() ? lobby_info.items : game_info.items;
    }

    Entities entities() const {
        return in_lobby_state() ? lobby_info.entities : game_info.entities;
    }

    void onUpdate(float dt) {
        TRACY_ZONE_SCOPED;
        for (auto rp : remote_players_NOT_SERIALIZED) {
            rp->update(dt);
        }
        if (in_lobby_state()) {
            lobby_info.ensure_generated_map(seed);
            lobby_info.onUpdate(dt);
        } else {
            game_info.ensure_generated_map(seed);
            game_info.onUpdate(dt);
        }
    }

    void onDraw(float dt) const {
        TRACY_ZONE_SCOPED;
        for (auto rp : remote_players_NOT_SERIALIZED) {
            // NOTE: we call the render directly ehre because level_info doesnt
            // own players
            // TODO why is he so large ..
            if (rp) system_manager::render_normal(rp, dt);
            if (!rp) log_warn("we have invalid remote players");
        }
        if (in_lobby_state()) {
            lobby_info.onDraw(dt);
        } else {
            game_info.onDraw(dt);
        }
    }

    void onDrawUI(float dt) {
        TRACY_ZONE_SCOPED;
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
    void ensure_generated_map() {
        // game_info.ensure_generated_map(seed);
    }
    void grab_things() {
        if (in_lobby_state()) {
            lobby_info.grab_things();
        } else {
            game_info.grab_things();
        }
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.object(lobby_info);
        s.object(game_info);
    }
};
