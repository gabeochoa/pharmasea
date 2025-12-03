#pragma once

#include "../ah.h"
#include "../components/can_hold_item.h"
#include "../components/has_day_night_timer.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "../entity_type.h"

namespace system_manager {

// System for processing grabber filter during in-round updates
struct ProcessGrabberFilterSystem : public afterhours::System<> {
    virtual ~ProcessGrabberFilterSystem() = default;

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

    // TODO use system filters
    virtual void for_each_with(Entity& entity, float) override {
        if (!check_type(entity, EntityType::FilteredGrabber)) return;
        if (entity.is_missing<CanHoldItem>()) return;
        CanHoldItem& canHold = entity.get<CanHoldItem>();
        if (canHold.empty()) return;

        // If we are holding something, then:
        // - either its already in the filter (and setting it wont be a big
        // deal)
        // - or we should set the filter

        EntityFilter& ef = canHold.get_filter();
        ef.set_filter_with_entity(canHold.const_item());
    }
};

}  // namespace system_manager
