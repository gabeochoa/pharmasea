
#pragma once

#include "../engine/log.h"
#include "ai_component.h"
#include "base_component.h"

struct AICloseTab : public AIComponent {
    AICloseTab() { set_cooldown(0.1f); }

    virtual ~AICloseTab() {}

    // TODO :BE: so many have this, can we make it an interface?
    [[nodiscard]] bool has_available_target() const {
        return target_id.has_value();
    }
    void unset_target() { target_id = {}; }
    void set_target(int id) { target_id = id; }
    [[nodiscard]] int id() const { return target_id.value(); }

    std::optional<int> target_id;

    vec2 position;

    float PayProcessingTime = -1;
    void set_PayProcessing_time(float pt) { PayProcessingTime = pt; }
    [[nodiscard]] bool PayProcessing(float dt) {
        PayProcessingTime -= dt;
        return PayProcessingTime <= 0.f;
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<AIComponent>{});
    }
};
