#pragma once

#include "../engine/log.h"
#include "ai_component.h"
#include "base_component.h"

struct AIDrinking : public AIComponent {
    virtual ~AIDrinking() {}

    [[nodiscard]] bool has_available_target() const {
        return target_pos.has_value();
    }

    void unset_target() { target_pos = {}; }
    void set_target(vec2 pos) { target_pos = pos; }
    [[nodiscard]] vec2 pos() const { return target_pos.value(); }

    std::optional<vec2> target_pos;

    float drinkTime;
    void set_drink_time(float pt) { drinkTime = pt; }
    [[nodiscard]] bool drink(float dt) {
        drinkTime -= dt;
        return drinkTime <= 0.f;
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<AIComponent>{});
    }
};
