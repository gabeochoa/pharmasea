
#pragma once

#include "../item.h"
#include "base_component.h"
#include "transform.h"

struct CustomHeldItemPosition : public BaseComponent {
    std::function<vec3(Transform&)> mutator;

    void init(std::function<vec3(Transform&)> mut) {
        // TODO enforce that to have this one you have to have a transform
        mutator = mut;
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
