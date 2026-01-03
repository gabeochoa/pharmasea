
#pragma once

#include "../entity_type.h"
#include "base_component.h"

struct HasProgression : public BaseComponent {
   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        (void) self;
        return archive(                      //
            static_cast<BaseComponent&>(self)  //
        );
    }
};
