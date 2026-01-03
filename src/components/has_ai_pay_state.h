#pragma once

#include "ai_line_wait.h"
#include "ai_takes_time.h"
#include "base_component.h"

struct HasAIPayState : public BaseComponent {
    AIWaitInQueueState line_wait{};
    AITakesTime timer{};

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.line_wait,                   //
            self.timer                        //
        );
    }
};

