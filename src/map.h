

#pragma once

#include "engine.h"
#include "engine/container_cast.h"
#include "engine/log.h"
#include "external_include.h"
//
#include "entityhelper.h"
#include "level_info.h"
#include "system/system_manager.h"

struct Map {
    bool showMinimap = false;

    LevelInfo game_info;

    Entities local_players_NOT_SERIALIZED;
    Entities remote_players_NOT_SERIALIZED;
    std::string seed;

    // This gets called on every network frame because
    // the serializer uses the default contructor
    Map() {}

    Map(const std::string& _seed) : seed(_seed) {
        // TODO this is needed for the items to be regenerated
        update_seed(seed);
    }

    void update_seed(const std::string& s) {
        log_info("changing seed from {} to {}", seed, s);
        seed = s;
        game_info.update_seed(s);
    }

    Entities entities() const { return game_info.entities; }

    void onUpdate(float dt) { _onUpdate(remote_players_NOT_SERIALIZED, dt); }

    void onUpdateLocalPlayers(float dt) {
        SystemManager::get().update_local_players(local_players_NOT_SERIALIZED,
                                                  dt);
    }

    void _onUpdate(std::vector<std::shared_ptr<Entity>> players, float dt) {
        TRACY_ZONE_SCOPED;
        // TODO add to debug overlay
        // log_info("num items {}", items().size());

        game_info.ensure_generated_map(seed);
        game_info.onUpdate(players, dt);
    }

    void onDraw(float dt) const {
        TRACY_ZONE_SCOPED;
        // TODO merge this into normal render pipeline
        SystemManager::get().render_entities(
            container_cast(remote_players_NOT_SERIALIZED,
                           "converting sp<RemotePlayer> to sp<Entity> as these "
                           "are not serialized and so not part of level info"),
            dt);

        game_info.onDraw(dt);
    }

    void onDrawUI(float dt) {
        TRACY_ZONE_SCOPED;

        // TODO merge this into normal render pipeline
        SystemManager::get().render_ui(
            container_cast(remote_players_NOT_SERIALIZED,
                           "converting sp<RemotePlayer> to sp<Entity> as these "
                           "are not serialized and so not part of level info"),
            dt);
        game_info.onDrawUI(dt);
    }

    [[nodiscard]] bool in_lobby_state() const {
        return GameState::get().is(game::State::Lobby) ||
               GameState::get().is_paused_in(game::State::Lobby);
    }

    // These are called before every "send_map_state" when server
    // sends everything over to clients
    void grab_things() { game_info.grab_things(); }

    // TODO do we need this
    void ensure_generated_map() {
        // game_info.ensure_generated_map(seed);
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.object(game_info);
        s.value1b(showMinimap);
    }
};
