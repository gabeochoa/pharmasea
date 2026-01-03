

#pragma once

#include "base_component.h"

struct IsSolid : public BaseComponent {
   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self)  //
        );
    }
};
