

#pragma once

#include "base_component.h"

struct IsUpgradeSpawned : public BaseComponent {
    virtual ~IsUpgradeSpawned() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
