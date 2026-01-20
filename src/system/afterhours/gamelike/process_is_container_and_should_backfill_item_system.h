#pragma once

#include "../../../ah.h"
#include "../../../components/can_hold_item.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../components/is_item_container.h"
#include "../../../entity_helper.h"
#include "../../../engine/statemanager.h"
#include "../afterhours_systems.h"
#include "../../core/system_manager.h"

namespace system_manager {

struct ProcessIsContainerAndShouldBackfillItemSystem
    : public afterhours::System<IsItemContainer, CanHoldItem> {
    virtual ~ProcessIsContainerAndShouldBackfillItemSystem() = default;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
            // Skip during transitions to avoid creating hundreds of items
            // before transition logic completes
            return !timer.needs_to_process_change;
        } catch (...) {
            return true;
        }
    }

    // TODO fold in function implementation
    virtual void for_each_with(Entity& entity, IsItemContainer& iic,
                               CanHoldItem& canHold, float dt) override {
        (void) iic;      // Unused parameter
        (void) canHold;  // Unused parameter
        process_is_container_and_should_backfill_item(entity, dt);
    }
};

}  // namespace system_manager