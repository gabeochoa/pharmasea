

#pragma once

#include "base_component.h"

struct CanBePushed : public BaseComponent {
    virtual ~CanBePushed() {}

    [[nodiscard]] vec3 pushed_force() const { return this->force; }
    void update(vec3 f) { this->force = f; }
    void update_x(float x) { this->force.x = x; }
    void update_y(float y) { this->force.y = y; }
    void update_z(float z) { this->force.z = z; }

   private:
    vec3 force{0.0, 0.0, 0.0};

    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this));
    }
};

CEREAL_REGISTER_TYPE(CanBePushed);
