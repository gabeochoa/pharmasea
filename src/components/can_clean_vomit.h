#pragma once

#include "base_component.h"

// Capability marker: entities with this component are allowed to run the
// CleanVomit AI behavior/state.
struct CanCleanVomit : public BaseComponent {
   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(static_cast<BaseComponent&>(self));
    }
};

