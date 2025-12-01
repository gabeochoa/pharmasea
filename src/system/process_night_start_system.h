#pragma once

#include "../ah.h"
#include "../components/has_day_night_timer.h"
#include "../engine/log.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "system_manager.h"

namespace system_manager {

// TODO eventually split this into a separate system for each night start logic

// System that processes night start logic when needs_to_process_change is true
// and is_nighttime is true
struct ProcessNightStartSystem : public afterhours::System<> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
            return timer.needs_to_process_change && timer.is_nighttime();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity& entity,
                               [[maybe_unused]] float dt) override {
        log_info("DAY ENDED");

        // Store setup
        store::cleanup_old_store_options();

        SystemManager::get().for_each_old([](Entity& entity) {
            day_night::on_day_ended(entity);

            // just in case theres anyone in the queue still, just
            // clear it before the customers start coming in
            reset_register_queue_when_leaving_inround(entity);

            close_buildings_when_night(entity);

            day_night::on_night_started(entity);

            // - TODO keeps respawning roomba, we should probably
            // not do that anymore...just need to clean it up at end
            // of day i guess or let him roam??
            release_mop_buddy_at_start_of_day(entity);

            delete_trash_when_leaving_planning(entity);
            // TODO
            // upgrade::on_round_started(entity, dt);
        });

        (void) entity;
        (void) dt;
    }
};

}  // namespace system_manager
