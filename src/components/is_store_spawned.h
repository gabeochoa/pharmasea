

#pragma once

#include "base_component.h"

struct IsStoreSpawned : public BaseComponent {
   private:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self)  //
        );
    }
};
