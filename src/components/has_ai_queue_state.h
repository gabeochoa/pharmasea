#pragma once

#include "../entity_ref.h"
#include "ai_line_wait.h"
#include "base_component.h"

struct HasAIQueueState : public BaseComponent {
    // Helps avoid register thrash (optional).
    EntityRef last_register{};
    int queue_index = -1;

    AIWaitInQueueState line_wait{};

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.last_register,               //
            self.queue_index,                 //
            self.line_wait                    //
        );
    }
};

