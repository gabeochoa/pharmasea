

#pragma once

#include "components/collects_user_input.h"
#include "engine.h"
#include "engine/container_cast.h"
#include "engine/log.h"
#include "external_include.h"
//
#include "entity_helper.h"
#include "level_info.h"
#include "system/system_manager.h"

struct Map {
    // Serialized
    bool showMinimap = false;
    LevelInfo game_info;

    // No serialized
    Entities local_players_NOT_SERIALIZED;
    Entities remote_players_NOT_SERIALIZED;
    std::string seed;
    bool showSeedInputBox = false;

    // This gets called on every network frame because
    // the serializer uses the default contructor
    Map() {}

    explicit Map(const std::string& _seed) : seed(_seed) {
        // TODO :NOTE: this is needed for the items to be regenerated
        update_seed(seed);
    }

    void update_map(const Map& new_map) {
        this->showMinimap = new_map.showMinimap;
    }

    void update_seed(const std::string& s) {
        seed = s;
        game_info.update_seed(s);
    }

    OptEntity get_remote_with_cui() {
        for (const auto& e : remote_players_NOT_SERIALIZED) {
            if (e->has<CollectsUserInput>()) {
                return *e;
            }
        }
        return {};
    }

    Entities entities() const { return game_info.entities; }

    void onUpdate(float dt) {  //
        _onUpdate(remote_players_NOT_SERIALIZED, dt);
    }

    void onUpdateLocalPlayers(float dt) {
        // TODO for right now the only thing this does is collect input
        // which we want to turn off until you close the box
        if (showSeedInputBox) return;
        SystemManager::get().update_local_players(local_players_NOT_SERIALIZED,
                                                  dt);
    }

    void _onUpdate(const std::vector<std::shared_ptr<Entity>>& players,
                   float dt) {
        TRACY_ZONE_SCOPED;
        // TODO :BE: add to debug overlay
        // log_info("num items {}", items().size());

        game_info.ensure_generated_map(seed);
        game_info.onUpdate(players, dt);
    }

    void onDraw(float dt) const {
        TRACY_ZONE_SCOPED;
        // TODO :INFRA: merge this into normal render pipeline
        SystemManager::get().render_entities(
            container_cast(remote_players_NOT_SERIALIZED,
                           "converting sp<RemotePlayer> to sp<Entity> as these "
                           "are not serialized and so not part of level info"),
            dt);

        game_info.onDraw(dt);
    }

    void onDrawUI(float dt) {
        TRACY_ZONE_SCOPED;

        // TODO :INFRA: merge this into normal render pipeline
        SystemManager::get().render_ui(
            container_cast(remote_players_NOT_SERIALIZED,
                           "converting sp<RemotePlayer> to sp<Entity> as these "
                           "are not serialized and so not part of level info"),
            dt);
        game_info.onDrawUI(dt);
    }

    // These are called before every "send_map_state" when server
    // sends everything over to clients
    void grab_things() { game_info.grab_things(); }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.object(game_info);
        s.value1b(showMinimap);
    }
};
