#pragma once

#include "ai_wait_in_queue_state.h"
#include "base_component.h"
#include "cooldown_info.h"
#include "is_ai_controlled.h"

struct HasAIBathroomState : public BaseComponent {
    AIWaitInQueueState line_wait{};
    CooldownInfo timer{};
    CooldownInfo floor_timer{};

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

