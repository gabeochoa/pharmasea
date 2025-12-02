#pragma once

#include "../ah.h"
#include "../components/has_day_night_timer.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "system_manager.h"

namespace system_manager {
void process_floor_markers(Entity& entity, float dt);
void reset_highlighted(Entity& entity, float dt);
void process_trigger_area(Entity& entity, float dt);
void process_nux_updates(Entity& entity, float dt);
void refetch_dynamic_model_names(Entity& entity, float dt);
void highlight_facing_furniture(Entity& entity, float dt);
void transform_snapper(Entity& entity, float dt);
void update_held_item_position(Entity& entity, float dt);
void update_held_furniture_position(Entity& entity, float dt);
void update_held_hand_truck_position(Entity& entity, float dt);
void update_visuals_for_settings_changer(Entity& entity, float dt);
void process_squirter(Entity& entity, float dt);
void process_soda_fountain(Entity& entity, float dt);
void process_trash(Entity& entity, float dt);
void delete_customers_when_leaving_inround(Entity& entity);
void run_timer(Entity& entity, float dt);
void process_pnumatic_pipe_pairing(Entity& entity, float dt);
void process_is_container_and_should_backfill_item(Entity& entity, float dt);
void pass_time_for_transaction_animation(Entity& entity, float dt);
void update_sophie(Entity& entity, float dt);
void process_is_container_and_should_update_item(Entity& entity, float dt);
void process_is_indexed_container_holding_incorrect_item(Entity& entity,
                                                         float dt);
void reset_customers_that_need_resetting(Entity& entity);
void process_grabber_items(Entity& entity, float dt);
void process_conveyer_items(Entity& entity, float dt);
void process_grabber_filter(Entity& entity, float dt);
void process_pnumatic_pipe_movement(Entity& entity, float dt);
void process_has_rope(Entity& entity, float dt);
void process_spawner(Entity& entity, float dt);
void reset_empty_work_furniture(Entity& entity, float dt);
void reduce_impatient_customers(Entity& entity, float dt);
void pass_time_for_active_fishing_games(Entity& entity, float dt);
void pop_out_when_colliding(Entity& entity, float dt);
void delete_floating_items_when_leaving_inround(Entity& entity);
void delete_held_items_when_leaving_inround(Entity& entity);
void reset_max_gen_when_after_deletion(Entity& entity);
void tell_customers_to_leave(Entity& entity);
void reset_toilet_when_leaving_inround(Entity& entity);
void reset_customer_spawner_when_leaving_inround(Entity& entity);
void update_new_max_customers(Entity& entity, float dt);
void reset_register_queue_when_leaving_inround(Entity& entity);
void close_buildings_when_night(Entity& entity);
void release_mop_buddy_at_start_of_day(Entity& entity);
void delete_trash_when_leaving_planning(Entity& entity);

namespace render_manager {
void update_character_model_from_index(Entity& entity, float dt);
void on_frame_start();
void render_walkable_spots(float dt);
void render(const Entity& entity, float dt, bool is_debug);
}  // namespace render_manager

namespace ai {
void process_(Entity& entity, float dt);
}  // namespace ai

namespace store {
void cart_management(Entity& entity, float dt);
void generate_store_options();
void open_store_doors();
void cleanup_old_store_options();
}  // namespace store

namespace day_night {
void on_night_ended(Entity& entity);
void on_day_started(Entity& entity);
void on_day_ended(Entity& entity);
void on_night_started(Entity& entity);
}  // namespace day_night

namespace upgrade {
void on_round_finished(Entity& entity, float dt);
void in_round_update(Entity& entity, float dt);
}  // namespace upgrade

// Simple proof-of-concept system: Timer system that processes entities with
// HasDayNightTimer
struct TimerSystem : public afterhours::System<HasDayNightTimer> {
    virtual ~TimerSystem() = default;

    virtual void for_each_with([[maybe_unused]] Entity& entity,
                               [[maybe_unused]] HasDayNightTimer& timer,
                               [[maybe_unused]] float dt) override {
        // This is called automatically for all entities with HasDayNightTimer
        // component We can add the timer logic here later For now, this is just
        // a proof of concept
    }
};

// Sixty FPS update system - processes entities at 60fps
struct SixtyFpsUpdateSystem : public afterhours::System<> {
    virtual ~SixtyFpsUpdateSystem() = default;

    // This system should run in all states (lobby, game, model test, etc.)
    // because it handles trigger areas and other essential updates
    // Note: We run every frame for better responsiveness (especially for
    // trigger areas), which is acceptable since these operations are
    // lightweight. The original ran at 60fps (when timePassed >= 0.016f), but
    // running every frame ensures trigger areas and other interactions feel
    // more responsive.
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with([[maybe_unused]] Entity& entity,
                               [[maybe_unused]] float dt) override {
        // All functions have been migrated to individual systems
    }
};

// Game-like update system - processes entities during game-like states
struct GameLikeUpdateSystem : public afterhours::System<> {
    virtual ~GameLikeUpdateSystem() = default;

    virtual bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    virtual void for_each_with(Entity& entity,
                               [[maybe_unused]] float dt) override {
        // Day/night transition logic has been moved to separate systems:
        // ComputeHasDayNightChanged, ProcessDayStartSystem,
        // ProcessNightStartSystem, and ResetHasDayNightChanged
        (void) entity;
        (void) dt;
    }
};

// Model test update system - processes entities during model test state
struct ModelTestUpdateSystem : public afterhours::System<> {
    virtual ~ModelTestUpdateSystem() = default;

    virtual bool should_run(const float) override {
        return GameState::get().is(game::State::ModelTest);
    }

    virtual void for_each_with(Entity& entity, float dt) override {
        // should move all the container functions into its own
        // function?
        process_is_container_and_should_update_item(entity, dt);
        // This one should be after the other container ones
        process_is_indexed_container_holding_incorrect_item(entity, dt);

        process_is_container_and_should_backfill_item(entity, dt);
    }
};

// In-round update system - processes entities during in-round (nighttime)
struct InRoundUpdateSystem : public afterhours::System<> {
    virtual ~InRoundUpdateSystem() = default;

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

    virtual void for_each_with(Entity& entity, float dt) override {
        reset_customers_that_need_resetting(entity);
        //
        process_grabber_items(entity, dt);
        process_conveyer_items(entity, dt);
        process_grabber_filter(entity, dt);
        process_pnumatic_pipe_movement(entity, dt);
        process_has_rope(entity, dt);
        // should move all the container functions into its own
        // function?
        process_is_container_and_should_update_item(entity, dt);
        // This one should be after the other container ones
        process_is_indexed_container_holding_incorrect_item(entity, dt);

        process_spawner(entity, dt);
        reset_empty_work_furniture(entity, dt);
        reduce_impatient_customers(entity, dt);

        pass_time_for_active_fishing_games(entity, dt);

        upgrade::in_round_update(entity, dt);
    }
};

}  // namespace system_manager
