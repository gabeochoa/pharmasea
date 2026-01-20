#pragma once

#include "../../../ah.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../engine/statemanager.h"
#include "../../../entity_helper.h"
#include "../../core/system_manager.h"
#include "../../helpers/store_management_helpers.h"

namespace system_manager {

struct CleanUpOldStoreOptionsSystem : public afterhours::System<> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
            return timer.needs_to_process_change && timer.is_bar_closed();
        } catch (...) {
            return false;
        }
    }
    virtual void once(float) override { store::cleanup_old_store_options(); }
};

}  // namespace system_manager