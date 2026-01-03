#pragma once

#include "ai_takes_time.h"
#include "base_component.h"

struct HasAIDrinkState : public BaseComponent {
    AITakesTime timer{};

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.timer                        //
        );
    }
};

