#pragma once

#include "../engine/graphics.h"
#include "base_component.h"

struct AIWaitInQueueState : public BaseComponent {
    bool has_set_position_before = false;
    vec2 position{};
    int last_line_position = -1;

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.has_set_position_before,     //
            self.position,                    //
            self.last_line_position           //
        );
    }
};

