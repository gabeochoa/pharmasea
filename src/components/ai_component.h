#pragma once

#include <functional>

#include "../engine/assert.h"
#include "../engine/log.h"
#include "../entity_helper.h"
#include "base_component.h"

struct AITarget {
    using ResetFn = std::function<void()>;
    using SuccessFn = std::function<void(Entity&)>;

    ResetFn reset;
    explicit AITarget(const ResetFn& resetFn) : reset(resetFn) {}

    [[nodiscard]] bool exists() const { return target_id.has_value(); }
    [[nodiscard]] bool missing() const { return !exists(); }

    void unset() { target_id = {}; }
    void set(int id) { target_id = id; }
    [[nodiscard]] int id() const { return target_id.value(); }

    std::optional<int> target_id;

    bool _find_target(const Entity& entity,
                      const SuccessFn& onFound = nullptr) {
        OptEntity closest = find_target(entity);

        // We couldnt find anything, for now just wait a second
        if (!closest) {
            reset();
            return false;
        }
        set(closest->id);
        if (onFound) onFound(closest.asE());
        return true;
    }

    virtual OptEntity find_target(const Entity& entity) = 0;

    bool find_if_missing(const Entity& entity,
                         const SuccessFn& onFound = nullptr) {
        if (missing()) {
            return _find_target(entity, onFound);
        }
        return true;
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
