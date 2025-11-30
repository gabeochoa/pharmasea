#pragma once

#include "../ah.h"
#include "../components/is_nux_manager.h"
#include "../components/is_round_settings_manager.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "../entity_query.h"
#include "system_manager.h"

namespace system_manager {

// Forward declaration for helper function in system_manager.cpp
bool _create_nuxes(Entity& entity);
void process_nux_updates(Entity& entity, float dt);

struct ProcessNuxUpdatesSystem
    : public afterhours::System<IsNuxManager, IsRoundSettingsManager> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsNuxManager& inm,
                               IsRoundSettingsManager&, float dt) override {
        // Tutorial isnt on so dont do any nuxes
        if (!entity.get<IsRoundSettingsManager>()
                 .interactive_settings.is_tutorial_active) {
            return;
        }

        // only generate the nux once you leave the lobby
        if (!GameState::get().is_game_like()) return;

        if (!inm.initialized) {
            bool init = _create_nuxes(entity);
            if (!init) return;
            inm.initialized = init;
        }

        // Forward to full implementation
        process_nux_updates(entity, dt);
    }
};

}  // namespace system_manager
