#pragma once

#include "../engine/graphics.h"
#include "base_component.h"

struct AIWaitInQueueState : public BaseComponent {
    // TODO: This is currently just a guard/diagnostic bit that prevents queue
    // helpers from running before we've actually joined a queue. Now that
    // queue movement uses HasAITargetLocation + HasAITargetEntity, consider
    // removing this and relying on those signals instead.
    bool has_set_position_before = false;
    int previous_line_index = -1;

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.has_set_position_before,     //
            self.previous_line_index          //
        );
    }
};

