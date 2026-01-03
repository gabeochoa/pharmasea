#pragma once

#include <functional>

#include "../components/ai_wait_in_queue_state.h"
#include "../components/can_pathfind.h"
#include "../components/has_ai_target_location.h"
#include "../components/has_waiting_queue.h"
#include "../components/transform.h"
#include "../engine/assert.h"
#include "../engine/log.h"
#include "../entity.h"

#include "ai_helper.h"

namespace system_manager::ai {

// ---- Queue/line helpers (system logic; queue state is data-only) ----
inline void line_reset(Entity& entity, AIWaitInQueueState& s) {
    s.has_set_position_before = false;
    s.previous_line_index = -1;
    s.queue_index = -1;
    if (entity.has<HasAITargetLocation>()) {
        entity.get<HasAITargetLocation>().pos.reset();
    }
}

inline void line_add_to_queue(Entity& entity, AIWaitInQueueState& s,
                              Entity& reg) {
    VALIDATE(reg.has<HasWaitingQueue>(),
             "Trying to add_to_queue for entity which doesn't have a waiting "
             "queue");
    HasWaitingQueue& hwq = reg.get<HasWaitingQueue>();
    int next_position = hwq.add_customer(entity).get_next_pos();
    HasAITargetLocation& tl = ensure_component<HasAITargetLocation>(entity);
    tl.pos = reg.get<Transform>().tile_infront((next_position + 1));
    s.has_set_position_before = true;
}

[[nodiscard]] inline int line_position_in_line(AIWaitInQueueState& s,
                                               Entity& reg,
                                               const Entity& entity) {
    VALIDATE(reg.has<HasWaitingQueue>(),
             "Trying to position_in_line for entity which doesn't have a "
             "waiting queue");
    const HasWaitingQueue& hwq = reg.get<HasWaitingQueue>();
    s.previous_line_index = hwq.get_customer_position(entity.id);
    s.queue_index = s.previous_line_index;
    return s.previous_line_index;
}

[[nodiscard]] inline bool line_can_move_up(const Entity& reg,
                                          const Entity& customer) {
    VALIDATE(reg.has<HasWaitingQueue>(),
             "Trying to can_move_up for entity which doesn't have a waiting "
             "queue");
    return reg.get<HasWaitingQueue>().matching_id(customer.id, 0);
}

// Returns true when the entity reaches the front of the line.
[[nodiscard]] inline bool line_try_to_move_closer(
    AIWaitInQueueState& s, Entity& reg, Entity& entity, float distance,
    const std::function<void()>& onReachedFront = nullptr) {
    if (!s.has_set_position_before) {
        log_error("AI line state: add_to_queue must be called first");
    }

    HasAITargetLocation& tl = ensure_component<HasAITargetLocation>(entity);
    if (!tl.pos.has_value()) {
        tl.pos = reg.get<Transform>().tile_directly_infront();
    }
    (void) entity.get<CanPathfind>().travel_toward(tl.pos.value(), distance);

    int spot_in_line = line_position_in_line(s, reg, entity);
    if (spot_in_line != 0) {
        if (!line_can_move_up(reg, entity)) {
            return false;
        }
        // Walk up one spot.
        tl.pos = reg.get<Transform>().tile_infront(spot_in_line);
        return false;
    }

    tl.pos = reg.get<Transform>().tile_directly_infront();
    if (onReachedFront) onReachedFront();
    return true;
}

inline void line_leave(AIWaitInQueueState& s, Entity& reg, const Entity& entity) {
    VALIDATE(reg.has<HasWaitingQueue>(),
             "Trying to leave_line for entity which doesn't have a waiting "
             "queue");
    int pos = line_position_in_line(s, reg, entity);
    if (pos == -1) return;
    reg.get<HasWaitingQueue>().erase(pos);
}

}  // namespace system_manager::ai

