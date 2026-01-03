#pragma once

#include "../entity_ref.h"
#include "ai_wait_in_queue_state.h"
#include "base_component.h"
#include "cooldown_info.h"

struct HasAIJukeboxState : public BaseComponent {
    EntityRef last_jukebox{};
    AIWaitInQueueState line_wait{};
    CooldownInfo timer{};

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.last_jukebox,                //
            self.line_wait,                   //
            self.timer                        //
        );
    }
};

