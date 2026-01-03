#pragma once

#include <functional>

#include "../engine/assert.h"
#include "../engine/log.h"
#include "../entity.h"
#include "base_component.h"
#include "can_pathfind.h"
#include "has_waiting_queue.h"

struct AIWaitInQueueState : public BaseComponent {
    bool has_set_position_before = false;
    vec2 position{};
    int last_line_position = -1;

    void reset_position_only() {
        has_set_position_before = false;
        last_line_position = -1;
        position = vec2{0, 0};
    }

    void add_to_queue(Entity& reg, const Entity& entity) {
        VALIDATE(reg.has<HasWaitingQueue>(),
                 "Trying to add_to_queue for entity which doesn't have a "
                 "waiting queue");
        HasWaitingQueue& hwq = reg.get<HasWaitingQueue>();
        int next_position = hwq.add_customer(entity).get_next_pos();
        position = reg.get<Transform>().tile_infront((next_position + 1));
        has_set_position_before = true;
    }

    [[nodiscard]] int position_in_line(Entity& reg, const Entity& entity) {
        VALIDATE(reg.has<HasWaitingQueue>(),
                 "Trying to position_in_line for entity which doesn't have a "
                 "waiting queue");
        const HasWaitingQueue& hwq = reg.get<HasWaitingQueue>();
        last_line_position = hwq.get_customer_position(entity.id);
        return last_line_position;
    }

    [[nodiscard]] bool can_move_up(const Entity& reg, const Entity& customer) {
        VALIDATE(reg.has<HasWaitingQueue>(),
                 "Trying to can_move_up for entity which doesn't have a "
                 "waiting queue");
        return reg.get<HasWaitingQueue>().matching_id(customer.id, 0);
    }

    // Returns true when the entity reaches the front of the line.
    [[nodiscard]] bool try_to_move_closer(
        Entity& reg, Entity& entity, float distance,
        const std::function<void()>& onReachedFront = nullptr) {
        if (!has_set_position_before) {
            log_error("AI line state: add_to_queue must be called first");
        }

        (void) entity.get<CanPathfind>().travel_toward(position, distance);

        int spot_in_line = position_in_line(reg, entity);
        if (spot_in_line != 0) {
            if (!can_move_up(reg, entity)) {
                return false;
            }
            // Walk up one spot.
            position = reg.get<Transform>().tile_infront(spot_in_line);
            return false;
        }

        position = reg.get<Transform>().tile_directly_infront();
        if (onReachedFront) onReachedFront();
        return true;
    }

    void leave_line(Entity& reg, const Entity& entity) {
        VALIDATE(reg.has<HasWaitingQueue>(),
                 "Trying to leave_line for entity which doesn't have a waiting "
                 "queue");
        int pos = position_in_line(reg, entity);
        if (pos == -1) return;
        reg.get<HasWaitingQueue>().erase(pos);
    }

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.has_set_position_before,     //
            self.position,                    //
            self.last_line_position           //
        );
    }
};

