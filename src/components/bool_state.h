
#pragma once

#include "base_component.h"

// Generic boolean state component template.
// Reduces boilerplate for simple on/off state components.
//
// Usage:
//   using CanBeGhostPlayer = BoolState<struct GhostPlayerTag>;
//
template<typename Tag>
struct BoolState : public BaseComponent {
    bool value = false;

    [[nodiscard]] bool is_set() const { return value; }
    [[nodiscard]] bool is_not_set() const { return !value; }
    void update(bool v) { value = v; }

    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self.value                          //
        );
    }
};
