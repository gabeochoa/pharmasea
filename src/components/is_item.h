

#pragma once

#include "base_component.h"

struct IsItem : public BaseComponent {
    enum HeldBy { NONE, ITEM, PLAYER, FURNITURE, CUSTOMER };

    virtual ~IsItem() {}

   private:
    HeldBy held_by = NONE;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(held_by);
    }
};
