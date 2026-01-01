
#pragma once

#include "base_component.h"

using OnChangeFn = std::function<void(Entity&, bool)>;

struct CanBeHighlighted : public BaseComponent {
    [[nodiscard]] bool is_highlighted() const { return highlighted; }
    [[nodiscard]] bool is_not_highlighted() const { return !is_highlighted(); }

    void update(Entity& entity, bool is_h) {
        if (highlighted != is_h && onchange) onchange(entity, is_h);

        highlighted = is_h;
    }

    void set_on_change(const OnChangeFn& cb) { onchange = cb; }

   private:
    bool highlighted;
    OnChangeFn onchange;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value1b(highlighted);
    }
};
