

#include "map.h"

#include "components/collects_user_input.h"
#include "engine.h"
#include "engine/container_cast.h"
#include "engine/log.h"
#include "external_include.h"
//
#include "entity_helper.h"
#include "level_info.h"
#include "system/system_manager.h"

void Map::update_map(const Map& new_map) {
    this->showMinimap = new_map.showMinimap;
}

void Map::update_seed(const std::string& s) {
    seed = s;
    game_info.update_seed(s);
}

OptEntity Map::get_remote_with_cui() {
    for (const auto& e : remote_players_NOT_SERIALIZED) {
        if (e->has<CollectsUserInput>()) {
            return *e;
        }
    }
    return {};
}

void Map::onUpdateLocalPlayers(float dt) {
    // TODO for right now the only thing this does is collect input
    // which we want to turn off until you close the box
    if (showSeedInputBox) return;
    SystemManager::get().update_local_players(local_players_NOT_SERIALIZED, dt);
}

void Map::onUpdateRemotePlayers(float dt) {
    SystemManager::get().update_remote_players(remote_players_NOT_SERIALIZED,
                                               dt);
}

void Map::_onUpdate(const std::vector<std::shared_ptr<Entity>>& players,
                    float dt) {
    TRACY_ZONE_SCOPED;
    // TODO :BE: add to debug overlay
    // log_info("num items {}", items().size());

    game_info.ensure_generated_map(seed);
    game_info.onUpdate(players, dt);
}

void Map::onDraw(float dt) const {
    TRACY_ZONE_SCOPED;
    // TODO :INFRA: merge this into normal render pipeline
    SystemManager::get().render_entities(remote_players_NOT_SERIALIZED, dt);

    game_info.onDraw(dt);
}

void Map::onDrawUI(float dt) {
    TRACY_ZONE_SCOPED;

    // TODO :INFRA: merge this into normal render pipeline
    SystemManager::get().render_ui(remote_players_NOT_SERIALIZED, dt);
    game_info.onDrawUI(dt);
}
