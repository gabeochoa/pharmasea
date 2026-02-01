#pragma once

#include "../../../ah.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../components/responds_to_day_night.h"
#include "../../../engine/statemanager.h"
#include "../../../entities/entity_helper.h"
#include "../../core/system_manager.h"

namespace system_manager {

struct OnDayEndedSystem : public afterhours::System<RespondsToDayNight> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
            return timer.needs_to_process_change && timer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity&, RespondsToDayNight& rtdn,
                               float) override {
        // TODO look into this
        rtdn.call_day_ended();
    }
};

}  // namespace system_manager