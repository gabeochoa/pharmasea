#pragma once

#include "ai_wait_in_queue_state.h"
#include "base_component.h"
#include "cooldown_info.h"

struct HasAIPayState : public BaseComponent {
    AIWaitInQueueState line_wait{};
    CooldownInfo timer{};

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self.line_wait,                     //
            self.timer                          //
        );
    }
};
