#pragma once

#include <functional>

#include "../engine/assert.h"
#include "../engine/log.h"
#include "../entity_helper.h"
#include "base_component.h"

struct AITarget {
    using FindTargetFn = std::function<OptEntity(const Entity&)>;
    using ResetFn = std::function<void()>;

    FindTargetFn ft = nullptr;
    ResetFn reset = nullptr;
    explicit AITarget(const FindTargetFn& ft, const ResetFn& rst)
        : ft(ft), reset(rst) {}

    [[nodiscard]] bool exists() const { return target_id.has_value(); }
    [[nodiscard]] bool missing() const { return !exists(); }

    void unset() { target_id = {}; }
    void set(int id) { target_id = id; }
    [[nodiscard]] int id() const { return target_id.value(); }

    std::optional<int> target_id;

    bool find_target(const Entity& entity) {
        VALIDATE(ft == nullptr,
                 "Using AITarget but forgot to set find function");
        VALIDATE(reset == nullptr,
                 "Using AITarget but forgot to set reset function");

        OptEntity closest = ft(entity);

        // We couldnt find anything, for now just wait a second
        if (!closest) {
            reset();
            return false;
        }
        set(closest->id);
        return true;
    }

    bool find_if_missing(const Entity& entity) {
        if (missing()) {
            return find_target(entity);
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
