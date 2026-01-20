#pragma once

#include "../../../ah.h"
#include "../../../components/can_hold_item.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../components/indexer.h"
#include "../../../components/is_item_container.h"
#include "../../../components/transform.h"
#include "../../../engine/statemanager.h"
#include "../../../entity_helper.h"
#include "../../core/system_manager.h"

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
            return hastimer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity& entity, Transform& transform,
                               IsItemContainer& iic, Indexer& indexer,
                               CanHoldItem& canHold, float) override {
        if (!iic.should_use_indexer()) return;

        // user didnt change the index so we are good to wait
        if (indexer.value_same_as_last_render()) return;

        // Delete the currently held item
        if (canHold.is_holding_item()) {
            OptEntity held_opt = canHold.item();
            if (held_opt) {
                held_opt.asE().cleanup = true;
            }
            canHold.update(nullptr, entity_id::INVALID);
        }

        // Backfill with the new indexed item
        if (!iic.hit_max()) {
            iic.increment();
            Entity& item = EntityHelper::createItem(iic.type(), transform.pos(),
                                                    indexer.value());
            canHold.update(item, entity.id);
        }

        indexer.mark_change_completed();
    }
};

}  // namespace system_manager