

#pragma once

#include "base_component.h"

struct CanGrabFromOtherFurniture : public BaseComponent {
   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        (void) self;
        return archive(                        //
            static_cast<BaseComponent&>(self)  //
        );
    }
};
