
#pragma once

#include "base_component.h"

struct IsSnappable : public BaseComponent {
    virtual ~IsSnappable() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
