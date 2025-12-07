#pragma once

#include "../ah.h"
#include "../components/has_day_night_timer.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "system_manager.h"

namespace system_manager {

void process_nux_updates(Entity& entity, float dt);
void process_is_container_and_should_backfill_item(Entity& entity, float dt);
void update_sophie(Entity& entity, float dt);
void process_is_container_and_should_update_item(Entity& entity, float dt);
void process_is_indexed_container_holding_incorrect_item(Entity& entity,
                                                         float dt);

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
void move_purchased_furniture();
}  // namespace store

namespace upgrade {
void in_round_update(Entity& entity, float dt);
}  // namespace upgrade

}  // namespace system_manager
