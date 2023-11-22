

#pragma once

#include "base_component.h"

struct IsStoreSpawned : public BaseComponent {
    virtual ~IsStoreSpawned() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
