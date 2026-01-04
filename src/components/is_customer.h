#pragma once

#include "base_component.h"

// Marker for customer-specific AI/entities. Intentionally minimal for now.
// Future: traits (VIP/thief), needs (bladder), preferences, etc.
struct IsCustomer : public BaseComponent {
   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(static_cast<BaseComponent&>(self));
    }
};

