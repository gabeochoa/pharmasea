

#pragma once

#include "base_component.h"

struct CanHighlightOthers : public BaseComponent {
    [[nodiscard]] float reach() const { return furniture_reach; }

   private:
    float furniture_reach = 1.80f;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        (void) self;
        return archive(                      //
            static_cast<BaseComponent&>(self)  //
        );
    }
};
