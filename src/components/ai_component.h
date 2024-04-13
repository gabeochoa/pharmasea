#pragma once

#include <functional>

#include "../engine/assert.h"
#include "../engine/log.h"
#include "../entity_helper.h"
#include "base_component.h"
#include "can_pathfind.h"
#include "has_waiting_queue.h"

struct AITarget {
    using ResetFn = std::function<void()>;
    using ValidateFn = std::function<bool(const Entity&)>;
    using SuccessFn = std::function<void(Entity&)>;

    ResetFn reset;
    explicit AITarget(const ResetFn& resetFn) : reset(resetFn) {}

    [[nodiscard]] bool exists() const { return target_id.has_value(); }
    [[nodiscard]] bool missing() const { return !exists(); }

    void unset() { target_id = {}; }
    void set(int id) { target_id = id; }
    [[nodiscard]] int id() const { return target_id.value(); }

    std::optional<int> target_id;

    [[nodiscard]] bool _find_target(const Entity& entity,
                                    const ValidateFn& validate = nullptr,
                                    const SuccessFn& onFound = nullptr) {
        OptEntity closest = find_target(entity);

        // We couldnt find anything, for now just wait a second
        if (!closest) {
            reset();
            return false;
        }

        if (validate) {
            bool success = validate(closest.asE());
            if (!success) return false;
        }

        set(closest->id);
        if (onFound) onFound(closest.asE());
        return true;
    }

    virtual OptEntity find_target(const Entity& entity) = 0;

    [[nodiscard]] bool find_if_missing(const Entity& entity,
                                       const ValidateFn& validate = nullptr,
                                       const SuccessFn& onFound = nullptr) {
        if (missing()) {
            return _find_target(entity, validate, onFound);
        }
        return true;
    }
};

struct AILineWait {
    using ResetFn = std::function<void()>;

    vec2 position;

   private:
    ResetFn reset;

   public:
    explicit AILineWait(const ResetFn& resetFn) : reset(resetFn) {}

    void add_to_queue(Entity& reg, const Entity& entity) {
        VALIDATE(reg.has<HasWaitingQueue>(),
                 "Trying to get-next-queue-pos for entity which doesnt have a "
                 "waiting queue ");
        HasWaitingQueue& hwq = reg.get<HasWaitingQueue>();
        int next_position = hwq.add_customer(entity).get_next_pos();
        position = reg.get<Transform>().tile_infront((next_position + 1));
    }

    [[nodiscard]] int position_in_line(Entity& reg, const Entity& entity) {
        VALIDATE(reg.has<HasWaitingQueue>(),
                 "Trying to pos-in-line for entity which doesnt have a "
                 "waiting queue ");
        const HasWaitingQueue& hwq = reg.get<HasWaitingQueue>();
        return hwq.has_matching_person(entity.id);
    }

    [[nodiscard]] bool can_move_up(const Entity& reg, const Entity& customer) {
        VALIDATE(reg.has<HasWaitingQueue>(),
                 "Trying to can-move-up for entity which doesnt "
                 "have a waiting queue ");
        return reg.get<HasWaitingQueue>().matching_id(customer.id, 0);
    }

    [[nodiscard]] bool try_to_move_closer(
        Entity& reg, Entity& entity, float distance,
        const std::function<void()>& onReachedFront = nullptr) {
        //
        (void) entity.get<CanPathfind>().travel_toward(position, distance);

        int spot_in_line = position_in_line(reg, entity);
        if (spot_in_line != 0) {
            // Waiting in line :)

            // TODO We didnt move so just wait a bit before trying again

            if (!can_move_up(reg, entity)) {
                // We cant move so just wait a bit before trying again
                log_trace("im just going to wait a bit longer");

                // Add the current job to the queue,
                // and then add the waiting job

                reset();
                return false;
            }
            // otherwise walk up one spot
            position = reg.get<Transform>().tile_infront(spot_in_line);

            return false;
        }

        position = reg.get<Transform>().tile_infront(1);
        if (onReachedFront) onReachedFront();
        return true;
    }

    void leave_line(Entity& reg, const Entity& entity) {
        // VALIDATE(customer, "entity passed to leave-line should not be null");
        // VALIDATE(reg, "register passed to leave-line should not be null");
        VALIDATE(reg.has<HasWaitingQueue>(),
                 "Trying to leave-line for entity which doesnt have a waiting "
                 "queue ");

        int pos = position_in_line(reg, entity);
        if (pos == -1) return;

        reg.get<HasWaitingQueue>().erase(pos);
    }
};

struct AIComponent : BaseComponent {
    // TODO :BE: what does cooldown do? when should we use it? do all AI need
    // it?
    float cooldown;
    float cooldownReset = 1.f;

    AIComponent() {
        // start with 0 for instant first frame use
        cooldown = 0;
    }
    void pass_time(float dt) {
        if (cooldown > 0) cooldown -= dt;
    }
    [[nodiscard]] bool ready() const { return cooldown <= 0; }
    void reset() { cooldown = cooldownReset; }
    void set_cooldown(float d) { cooldownReset = d; }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
