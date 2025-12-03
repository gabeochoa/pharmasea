#include "../ah.h"
#include "../components/can_hold_item.h"
#include "../components/has_day_night_timer.h"
#include "../components/indexer.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "system_manager.h"

namespace system_manager {

struct ProcessIsIndexedContainerHoldingIncorrectItemSystem
    : public afterhours::System<Indexer, CanHoldItem> {
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

    virtual void for_each_with(Entity&, Indexer& indexer, CanHoldItem& canHold,
                               float) override {
        // This function is for when you have an indexed container and you put
        // the item back in but the index had changed. you put the item back in
        // but the index had changed.
        // in this case we need to clear the item because otherwise
        // they will both live there which will cause overlap and grab issues.

        if (canHold.empty()) return;

        int current_value = indexer.value();
        int item_value = canHold.item().get<HasSubtype>().get_type_index();

        if (current_value != item_value) {
            canHold.item().cleanup = true;
            canHold.update(nullptr, -1);
        }
    }
};

}  // namespace system_manager