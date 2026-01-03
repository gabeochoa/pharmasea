#pragma once

#include "base_component.h"

// Currently just a marker scratch component for the CleanVomit behavior.
// Targeting is stored in HasAITargetEntity.
struct HasAICleanVomitState : public BaseComponent {
   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(static_cast<BaseComponent&>(self));
    }
};

