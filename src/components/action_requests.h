#pragma once

#include "base_component.h"

struct ActionRequests : public BaseComponent {
    bool pickup = false;
    bool rotate_furniture = false;
    bool handtruck_interact = false;
    float do_work = 0.f;

    void reset() {
        pickup = false;
        rotate_furniture = false;
        handtruck_interact = false;
        do_work = 0.f;
    }

    [[nodiscard]] bool any() const {
        return pickup || rotate_furniture || handtruck_interact ||
               do_work > 0.f;
    }

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                        //
            static_cast<BaseComponent&>(self),  //
            self.pickup,                        //
            self.rotate_furniture,              //
            self.handtruck_interact,            //
            self.do_work                         //
        );
    }
};
