

#pragma once

#include "base_component.h"

struct CanGrabFromOtherFurniture : public BaseComponent {
    virtual ~CanGrabFromOtherFurniture() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
