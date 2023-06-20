
#pragma once

#include "../item.h"
#include "base_component.h"

struct CanBeHighlighted : public BaseComponent {
    virtual ~CanBeHighlighted() {}

    [[nodiscard]] bool is_highlighted() const { return highlighted; }
    [[nodiscard]] bool is_not_highlighted() const { return !is_highlighted(); }

    void update(bool is_h) { highlighted = is_h; }

   private:
    bool highlighted;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value1b(highlighted);
    }
};
