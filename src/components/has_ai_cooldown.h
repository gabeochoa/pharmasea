#pragma once

#include "base_component.h"
#include "cooldown_info.h"

struct HasAICooldown : public BaseComponent {
    CooldownInfo cooldown{};

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self.cooldown                       //
        );
    }
};
