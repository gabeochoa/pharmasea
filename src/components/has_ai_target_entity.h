#pragma once

#include "../entities/entity_ref.h"
#include "base_component.h"

struct HasAITargetEntity : public BaseComponent {
    EntityRef entity{};

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self.entity                         //
        );
    }
};
