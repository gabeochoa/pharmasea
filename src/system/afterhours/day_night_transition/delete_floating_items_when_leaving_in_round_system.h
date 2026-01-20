#pragma once

#include "../../../ah.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../components/can_hold_item.h"
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

    inline void delete_held_items_when_leaving_inround(Entity& entity) {
        if (entity.is_missing<CanHoldItem>()) return;

        CanHoldItem& canHold = entity.get<CanHoldItem>();
        if (canHold.empty()) return;

        // Mark it as deletable
        // let go of the item
        OptEntity held_opt = canHold.item();
        if (held_opt) {
            held_opt.asE().cleanup = true;
        }
        canHold.update(nullptr, entity_id::INVALID);
    }

    inline void reset_max_gen_when_after_deletion(Entity& entity) {
        if (entity.is_missing<CanHoldItem>()) return;
        if (entity.is_missing<IsItemContainer>()) return;

        const CanHoldItem& canHold = entity.get<CanHoldItem>();
        // If something wasnt deleted, then just ignore it for now
        if (canHold.is_holding_item()) return;

        entity.get<IsItemContainer>().reset_generations();
    }

    virtual void for_each_with(Entity& entity, IsItem& ii, float) override {
        // Its being held by something so we'll get it in the function below
        if (ii.is_held()) return;

        // Skip the mop buddy for now
        if (check_type(entity, EntityType::MopBuddy)) return;

        // mark it for cleanup
        entity.cleanup = true;

        // TODO these we likely no longer need to do
        if (false) {
            delete_held_items_when_leaving_inround(entity);

            // I dont think we want to do this since we arent
            // deleting anything anymore maybe there might be a
            // problem with spawning a simple syurup in the
            // store??
            reset_max_gen_when_after_deletion(entity);
        }
    }
};

}  // namespace system_manager