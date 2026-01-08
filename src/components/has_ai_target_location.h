#pragma once

#include <optional>

#include "../engine/graphics.h"
#include "base_component.h"

struct HasAITargetLocation : public BaseComponent {
    std::optional<vec2> pos{};

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self.pos                            //
        );
    }
};
