

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

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.object(force);
    }
};
