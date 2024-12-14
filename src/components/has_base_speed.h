

#pragma once

#include "base_component.h"

struct HasBaseSpeed : public BaseComponent {
    virtual ~HasBaseSpeed() {}

    [[nodiscard]] float speed() const { return base_speed; }

    void update(float spd) { base_speed = spd; }

   private:
    float base_speed = 1.f;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this));
    }
};

CEREAL_REGISTER_TYPE(HasBaseSpeed);
