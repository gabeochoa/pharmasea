#pragma once

#include "../ah.h"

namespace system_manager {

// System for processing is_container_and_should_update_item during in-round
// updates
struct ProcessIsContainerAndShouldUpdateItemSystem
    : public afterhours::System<Transform, IsItemContainer, Indexer,
                                CanHoldItem> {
    virtual ~ProcessIsContainerAndShouldUpdateItemSystem() = default;

    virtual bool should_run(const float) override {
        // Model Test should always run this
        if (GameState::get().is(game::State::ModelTest)) return true;

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

    virtual void for_each_with(Entity& entity, Transform& transform,
                               IsItemContainer& iic, Indexer& indexer,
                               CanHoldItem& canHold, float) override {
        if (!iic.should_use_indexer()) return;
        // user didnt change the index so we are good to wait
        if (indexer.value_same_as_last_render()) return;

        // Delete the currently held item
        if (canHold.is_holding_item()) {
            canHold.item().cleanup = true;
            canHold.update(nullptr, -1);
        }

        auto pos = transform.pos();
        backfill_empty_container(iic.type(), entity, pos, indexer.value());
        indexer.mark_change_completed();
    }
};

}  // namespace system_manager
