

#pragma once

#include "base_component.h"

struct IsItem : public BaseComponent {
    virtual ~IsItem() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
