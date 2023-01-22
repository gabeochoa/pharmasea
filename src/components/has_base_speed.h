

#pragma once

#include "base_component.h"

struct HasBaseSpeed : public BaseComponent {
    virtual ~HasBaseSpeed() {}

    [[nodiscard]] float speed() const { return base_speed; }

    void update(float spd) { base_speed = spd; }

   private:
    float base_speed = 1.f;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value4b(base_speed);
    }
};
