
#pragma once

#include <optional>

#include "../../building_locations.h"
#include "../../components/ai_wait_in_queue_state.h"
#include "../../components/can_pathfind.h"
#include "../../components/is_ai_controlled.h"
#include "../../entity.h"
#include "../../entity_query.h"

namespace system_manager::ai {

bool validate_drink_order(const Entity& customer, Drink orderedDrink,
                          Item& madeDrink);
float get_speed_for_entity(Entity& entity);

// Used by the bathroom override system. This is intentionally exposed so the
// override can live outside the monolithic AI processor.
[[nodiscard]] bool needs_bathroom_now(Entity& entity);

// Rate-limit AI work in states that don't need per-frame processing.
[[nodiscard]] bool ai_tick_with_cooldown(Entity& entity, float dt,
                                         float reset_to_seconds);

[[nodiscard]] std::optional<vec2> pick_random_walkable_near(const Entity& e,
                                                            int attempts = 50);

[[nodiscard]] std::optional<vec2> pick_random_walkable_in_building(
    const Entity& e, const Building& building, int attempts = 50);

// ---- Queue/line helpers (system logic; queue state is data-only) ----
void line_reset(Entity& entity, AIWaitInQueueState& s);
void line_add_to_queue(Entity& entity, AIWaitInQueueState& s, Entity& reg);
[[nodiscard]] int line_position_in_line(AIWaitInQueueState& s, Entity& reg,
                                        const Entity& entity);
[[nodiscard]] bool line_can_move_up(const Entity& reg, const Entity& customer);
[[nodiscard]] bool line_try_to_move_closer(
    AIWaitInQueueState& s, Entity& reg, Entity& entity, float distance,
    const std::function<void()>& onReachedFront = nullptr);
void line_leave(AIWaitInQueueState& s, Entity& reg, const Entity& entity);

}  // namespace system_manager::ai
