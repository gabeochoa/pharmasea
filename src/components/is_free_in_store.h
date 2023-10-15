
#pragma once

#include "base_component.h"

struct IsFreeInStore : public BaseComponent {
    virtual ~IsFreeInStore() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
