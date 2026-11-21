

#pragma once

#include "base_component.h"

struct CanBePushed : public BaseComponent {
    [[nodiscard]] vec3 pushed_force() const { return this->force; }
    void update(vec3 f) { this->force = f; }
    void update_x(float x) { this->force.x = x; }
    void update_y(float y) { this->force.y = y; }
    void update_z(float z) { this->force.z = z; }

   private:
    vec3 force{0.0, 0.0, 0.0};

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        (void) self;
        return archive(                        //
            static_cast<BaseComponent&>(self)  //
        );
    }
};
