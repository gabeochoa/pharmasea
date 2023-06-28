

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

    std::vector<std::shared_ptr<Entity>> remote_players_NOT_SERIALIZED;
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

    Entities entities() const {
        return in_lobby_state() ? lobby_info.entities : game_info.entities;
    }

    void onUpdate(float dt) { _onUpdate(remote_players_NOT_SERIALIZED, dt); }

    void _onUpdate(std::vector<std::shared_ptr<Entity>> players, float dt) {
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

    void onDraw(float dt) const {
        TRACY_ZONE_SCOPED;
        SystemManager::get().render_entities(
            container_cast(remote_players_NOT_SERIALIZED,
                           "converting sp<RemotePlayer> to sp<Entity> as these "
                           "are not serialized and so not part of level info"),
            dt);

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
