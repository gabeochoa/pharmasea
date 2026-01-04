
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

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.highlighted                 //
        );
    }
};
