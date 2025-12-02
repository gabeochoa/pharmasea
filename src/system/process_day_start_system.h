#pragma once

#include "../ah.h"
#include "../components/has_day_night_timer.h"
#include "../engine/log.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "system_manager.h"

namespace system_manager {

// TODO eventually split this into a separate system for each day start logic

// System that processes day start logic when needs_to_process_change is true
// and is_daytime is true
struct ProcessDayStartSystem : public afterhours::System<> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
            return timer.needs_to_process_change && timer.is_daytime();
        } catch (...) {
            return false;
        }
    }

    virtual void once(float dt) override {
        log_info("DAY STARTED");

        // Store setup
        store::generate_store_options();
        store::open_store_doors();

        // Process day start logic for all entities
        SystemManager::get().for_each_old([dt](Entity& entity) {
            day_night::on_night_ended(entity);
            day_night::on_day_started(entity);

            delete_floating_items_when_leaving_inround(entity);

            // TODO these we likely no longer need to do
            if (false) {
                delete_held_items_when_leaving_inround(entity);

                // I dont think we want to do this since we arent
                // deleting anything anymore maybe there might be a
                // problem with spawning a simple syurup in the
                // store??
                reset_max_gen_when_after_deletion(entity);
            }

            tell_customers_to_leave(entity);

            // TODO we want you to always have to clean >:)
            // but we need some way of having the customers
            // finishe the last job they were doing (as long as it
            // isnt ordering) and then leaving, otherwise the toilet
            // is stuck "inuse" when its really not
            reset_toilet_when_leaving_inround(entity);

            reset_customer_spawner_when_leaving_inround(entity);

            // Handle updating all the things that rely on
            // progression
            update_new_max_customers(entity, dt);

            upgrade::on_round_finished(entity, dt);
        });
    }
};

}  // namespace system_manager
