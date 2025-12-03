#pragma once

#include "../ah.h"
#include "../components/has_day_night_timer.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "system_manager.h"

namespace system_manager {

// These functions are still in system_manager.cpp and called from *_system.h files
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
void move_purchased_furniture();
}  // namespace store

namespace upgrade {
void in_round_update(Entity& entity, float dt);
}  // namespace upgrade

}  // namespace system_manager
