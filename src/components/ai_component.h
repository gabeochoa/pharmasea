#pragma once

#include "../engine/log.h"
#include "base_component.h"

struct AIComponent : BaseComponent {
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
