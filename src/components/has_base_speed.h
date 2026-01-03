

#pragma once

#include "base_component.h"

struct HasBaseSpeed : public BaseComponent {
    [[nodiscard]] float speed() const { return base_speed; }

    void update(float spd) { base_speed = spd; }

   private:
    float base_speed = 1.f;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        // We dont serialize because its fully serverside
        (void) self;
        return archive(                      //
            static_cast<BaseComponent&>(self)  //
        );
    }
};
