
#pragma once

#include "base_component.h"

struct CanBeHighlighted : public BaseComponent {
    [[nodiscard]] bool is_highlighted() const { return highlighted; }
    [[nodiscard]] bool is_not_highlighted() const { return !is_highlighted(); }

    void update(bool is_h) { highlighted = is_h; }

   private:
    bool highlighted;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self.highlighted                    //
        );
    }
};
