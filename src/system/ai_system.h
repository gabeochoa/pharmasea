
#pragma once

#include "../components/can_pathfind.h"
#include "../components/is_ai_controlled.h"
#include "../entity.h"
#include "../entity_query.h"

namespace system_manager::ai {

bool validate_drink_order(const Entity& customer, Drink orderedDrink,
                          Item& madeDrink);
float get_speed_for_entity(Entity& entity);

// Used by the bathroom override system. This is intentionally exposed so the
// override can live outside the monolithic AI processor.
[[nodiscard]] bool needs_bathroom_now(Entity& entity);

// ---- Per-state AI behavior steps (called by afterhours systems) ----
void process_state_wander(Entity& entity, IsAIControlled& ctrl, float dt);
void process_state_queue_for_register(Entity& entity, float dt);
void process_state_at_register_wait_for_drink(Entity& entity, float dt);
void process_state_drinking(Entity& entity, float dt);
void process_state_pay(Entity& entity, float dt);
void process_state_play_jukebox(Entity& entity, float dt);
void process_state_bathroom(Entity& entity, float dt);
void process_state_clean_vomit(Entity& entity, float dt);
void process_state_leave(Entity& entity, float dt);

}  // namespace system_manager::ai
