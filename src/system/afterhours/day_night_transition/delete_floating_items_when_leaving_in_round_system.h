#pragma once

#include "../../../ah.h"
#include "../../../components/can_hold_item.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../components/is_item.h"
#include "../../../components/is_item_container.h"
#include "../../../engine/statemanager.h"
#include "../../../entity_helper.h"
#include "../../core/system_manager.h"

namespace system_manager {

struct DeleteFloatingItemsWhenLeavingInRoundSystem
    : public afterhours::System<IsItem> {
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

    virtual void for_each_with(Entity& entity, IsItem& ii, float) override {
        // Its being held by something so we'll get it in the function below
        if (ii.is_held()) return;

        // Skip the mop buddy for now
        if (check_type(entity, EntityType::MopBuddy)) return;

        // mark it for cleanup
        entity.cleanup = true;
    }
};

}  // namespace system_manager