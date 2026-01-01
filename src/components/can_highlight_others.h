

#pragma once

#include "base_component.h"

struct CanHighlightOthers : public BaseComponent {
    [[nodiscard]] float reach() const { return furniture_reach; }

   private:
    float furniture_reach = 1.80f;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
