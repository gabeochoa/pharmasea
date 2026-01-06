#pragma once

#include "base_component.h"
#include "cooldown_info.h"

struct HasAIDrinkState : public BaseComponent {
    CooldownInfo timer{};

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self.timer                          //
        );
    }
};
