#pragma once

#include "../../../ah.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../engine/statemanager.h"
#include "../../../entities/entity_helper.h"
#include "../../core/system_manager.h"

namespace system_manager {

// System that resets the needs_to_process_change flag after all transition
// systems have run
struct ResetHasDayNightChanged : public afterhours::System<HasDayNightTimer> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
            // Only run when the flag is set (meaning transition systems ran)
            return timer.needs_to_process_change;
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity&, HasDayNightTimer& timer,
                               float) override {
        // Reset the flag after all transition systems have run
        timer.needs_to_process_change = false;
    }
};

}  // namespace system_manager