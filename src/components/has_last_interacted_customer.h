
#pragma once

#include "../entity_ref.h"
#include "base_component.h"

struct HasLastInteractedCustomer : public BaseComponent {
    EntityRef customer{};

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self.customer                       //
        );
    }
};
