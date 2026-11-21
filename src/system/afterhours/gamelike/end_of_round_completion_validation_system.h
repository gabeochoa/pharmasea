#pragma once

#include "../../../ah.h"
#include "../../../components/collects_customer_feedback.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../engine/runtime_globals.h"
#include "../../../engine/statemanager.h"
#include "../../../globals.h"
#include "../../core/system_manager.h"

namespace system_manager {

namespace sophie {
// Forward declarations for sophie namespace functions
void customers_in_store(Entity& entity);
void holding_stolen_item(Entity& entity);
void garbage_in_store(Entity& entity);
void bar_not_clean(Entity& entity);
void overlapping_furniture(Entity& entity);
void forgot_item_in_spawn_area(Entity& entity);
void deleting_item_needed_for_recipe(Entity& entity);
void lightweight_map_validation(Entity& entity);
}  // namespace sophie

struct EndOfRoundCompletionValidationSystem
    : public afterhours::System<HasDayNightTimer, CollectsCustomerFeedback,
                                afterhours::tags::All<EntityType::Sophie>> {
    virtual bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    virtual void for_each_with(Entity& entity, HasDayNightTimer& ht,
                               CollectsCustomerFeedback& feedback,
                               float dt) override {
        const auto debug_mode_on = globals::debug_ui_enabled();

        // TODO i dont like that this is copy paste from layers/round_end
        if (SystemManager::get().is_bar_closed() &&
            ht.get_current_length() > 0 && !debug_mode_on)
            return;

        if (!feedback.waiting_time_pass(dt)) {
            return;
        }

        // doing it this way so that if we wanna make them return bool itll be
        // easy
        typedef std::function<void(Entity&)> WaitingFn;

        std::vector<WaitingFn> fns{
            sophie::customers_in_store,   //
            sophie::holding_stolen_item,  //
            sophie::garbage_in_store,     //
            // sophie::player_holding_furniture,         //
            sophie::bar_not_clean,                    //
            sophie::overlapping_furniture,            //
            sophie::forgot_item_in_spawn_area,        //
            sophie::deleting_item_needed_for_recipe,  //
            sophie::lightweight_map_validation,
        };

        for (const auto& fn : fns) {
            fn(entity);
        }
    }
};

}  // namespace system_manager