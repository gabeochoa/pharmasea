#pragma once

#include "../../../ah.h"
#include "../../../components/can_hold_item.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../components/has_subtype.h"
#include "../../../components/indexer.h"
#include "../../../engine/statemanager.h"
#include "../../../entity_helper.h"
#include "../../core/system_manager.h"

namespace system_manager {

// System for processing indexed containers holding incorrect items during
// in-round updates
struct ProcessIsIndexedContainerHoldingIncorrectItemSystem
    : public afterhours::System<Indexer, CanHoldItem> {
    virtual ~ProcessIsIndexedContainerHoldingIncorrectItemSystem() = default;

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

    // This handles when you have an indexed container and you put the
    // item back in but the index had changed.
    // We need to clear the item because otherwise they will both
    // live there which will cause overlap and grab issues.
    virtual void for_each_with(Entity&, Indexer& indexer, CanHoldItem& canHold,
                               float) override {
        if (canHold.empty()) return;

        int current_value = indexer.value();
        OptEntity held_opt = canHold.item();
        if (!held_opt) {
            canHold.update(nullptr, entity_id::INVALID);
            return;
        }
        int item_value = held_opt.asE().get<HasSubtype>().get_type_index();

        if (current_value != item_value) {
            held_opt.asE().cleanup = true;
            canHold.update(nullptr, entity_id::INVALID);
        }
    }
};

}  // namespace system_manager