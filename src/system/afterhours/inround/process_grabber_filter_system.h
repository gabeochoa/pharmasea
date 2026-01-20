#pragma once

#include "../../../ah.h"
#include "../../../components/can_hold_item.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../engine/statemanager.h"
#include "../../../entity_helper.h"
#include "../../core/system_manager.h"

namespace system_manager {

struct ProcessGrabberFilterSystem
    : public afterhours::System<
          CanHoldItem, afterhours::tags::All<EntityType::FilteredGrabber>> {
    virtual ~ProcessGrabberFilterSystem() = default;

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

    virtual void for_each_with(Entity& entity, CanHoldItem& canHold,
                               float) override {
        if (canHold.empty()) return;

        // If we are holding something, then:
        // - either its already in the filter (and setting it wont be a big
        // deal)
        // - or we should set the filter

        OptEntity held_opt = canHold.item();
        if (!held_opt) {
            canHold.update(nullptr, entity.id);
            return;
        }
        EntityFilter& ef = canHold.get_filter();
        ef.set_filter_with_entity(held_opt.asE());
    }
};

}  // namespace system_manager