

#pragma once

#include "../item.h"
#include "base_component.h"

struct CanHighlightOthers : public BaseComponent {
    virtual ~CanHighlightOthers() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
