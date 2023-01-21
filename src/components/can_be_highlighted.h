
#pragma once

#include "../item.h"
#include "base_component.h"

struct CanBeHighlighted : public BaseComponent {
    virtual ~CanBeHighlighted() {}

    bool is_highlighted;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value1b(is_highlighted);
    }
};
