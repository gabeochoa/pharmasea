#pragma once

#include "../engine/graphics.h"

namespace zpp::bits {
struct access;
}

// NOTE: This is intentionally NOT a top-level ECS component.
// It is embedded inside `HasAIQueueState` (the actual ECS component) to keep
// queue runtime state grouped, without doing "component inside component".
struct AIWaitInQueueState {
    // TODO: This is currently just a guard/diagnostic bit that prevents queue
    // helpers from running before we've actually joined a queue. Now that
    // queue movement uses HasAITargetLocation + HasAITargetEntity, consider
    // removing this and relying on those signals instead.
    bool has_set_position_before = false;
    // Index we were at on the previous update tick (0 = front).
    int previous_line_index = -1;
    // Current known index in the queue (0 = front).
    int queue_index = -1;

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            self.has_set_position_before,     //
            self.previous_line_index,         //
            self.queue_index                  //
        );
    }
};

