
#pragma once

#include "../entity_type.h"
#include "base_component.h"

struct HasProgression : public BaseComponent {
    virtual ~HasProgression() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
