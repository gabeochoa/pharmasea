#pragma once

#include "ai_line_wait.h"
#include "ai_takes_time.h"
#include "base_component.h"
#include "is_ai_controlled.h"

struct HasAIBathroomState : public BaseComponent {
    AILineWaitState line_wait{};
    AITakesTime timer{};
    AITakesTime floor_timer{};

    // Where to go when finished using the bathroom.
    IsAIControlled::State next_state = IsAIControlled::State::Wander;

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.line_wait,                   //
            self.timer,                       //
            self.floor_timer,                 //
            self.next_state                   //
        );
    }
};

