#pragma once

#include "../../../ah.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../components/has_fishing_game.h"
#include "../../../entity_helper.h"
#include "../../core/system_manager.h"

namespace system_manager {

namespace inround {

struct PassTimeForActiveFishingGamesSystem
    : public afterhours::System<HasFishingGame> {
    virtual ~PassTimeForActiveFishingGamesSystem() = default;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity&, HasFishingGame& fishingGame,
                               float dt) override {
        fishingGame.pass_time(dt);
    }
};

}  // namespace inround

}  // namespace system_manager