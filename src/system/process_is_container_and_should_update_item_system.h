#pragma once

#include "../ah.h"
#include "../components/can_hold_item.h"
#include "../components/indexer.h"
#include "../components/is_item_container.h"
#include "../components/transform.h"
#include "../entity_helper.h"
#include "../entity_query.h"
#include "afterhours_systems.h"
#include "system_manager.h"

namespace system_manager {

// System for processing containers that should update items during in-round
// updates
struct ProcessIsContainerAndShouldUpdateItemSystem
    : public afterhours::System<Transform, IsItemContainer, Indexer,
                                CanHoldItem> {
    virtual ~ProcessIsContainerAndShouldUpdateItemSystem() = default;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            // Don't run during transitions to avoid spawners creating entities
            // before transition logic completes
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_nighttime();
        } catch (...) {
            return false;
        }
    }

    // TODO fold in function implementation
    virtual void for_each_with(Entity& entity, Transform& transform,
                               IsItemContainer& iic, Indexer& indexer,
                               CanHoldItem& canHold, float dt) override {
        (void) transform;  // Unused parameter
        (void) iic;        // Unused parameter
        (void) indexer;    // Unused parameter
        (void) canHold;    // Unused parameter
        process_is_container_and_should_update_item(entity, dt);
    }
};

}  // namespace system_manager