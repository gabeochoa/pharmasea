

#pragma once

#include "base_component.h"

struct IsSolid : public BaseComponent {
    virtual ~IsSolid() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
