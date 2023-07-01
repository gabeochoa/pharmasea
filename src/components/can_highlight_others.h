

#pragma once

#include "../item.h"
#include "base_component.h"

struct CanHighlightOthers : public BaseComponent {
    virtual ~CanHighlightOthers() {}

    [[nodiscard]] float reach() const { return furniture_reach; }

   private:
    float furniture_reach = 1.80f;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
