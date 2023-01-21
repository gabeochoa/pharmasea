

#pragma once

#include "base_component.h"

struct CanBePushed : public BaseComponent {
    virtual ~CanBePushed() {}

    vec3 pushed_force{0.0, 0.0, 0.0};

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.object(pushed_force);
    }
};
