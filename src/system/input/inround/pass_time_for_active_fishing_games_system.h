#pragma once

#include "../../../ah.h"
#include "../../../components/has_fishing_game.h"
#include "../../core/system_manager.h"
#include "../../system_utilities.h"

namespace system_manager {

namespace inround {

struct PassTimeForActiveFishingGamesSystem
    : public afterhours::System<HasFishingGame> {
    virtual ~PassTimeForActiveFishingGamesSystem() = default;

    virtual bool should_run(const float) override {
        return system_utils::should_run_inround_system();
    }

    virtual void for_each_with(Entity&, HasFishingGame& fishingGame,
                               float dt) override {
        fishingGame.pass_time(dt);
    }
};

}  // namespace inround

}  // namespace system_manager