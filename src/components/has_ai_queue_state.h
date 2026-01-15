#pragma once

#include "../entity_ref.h"
#include "ai_wait_in_queue_state.h"
#include "base_component.h"

struct HasAIQueueState : public BaseComponent {
    // Helps avoid register thrash (optional).
    EntityRef last_register{};

    AIWaitInQueueState line_wait{};

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self.last_register,                 //
            self.line_wait                      //
        );
    }
};
