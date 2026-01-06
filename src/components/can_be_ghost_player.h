
#pragma once

#include "base_component.h"

// TODO is this actually used anymore
struct CanBeGhostPlayer : public BaseComponent {
    [[nodiscard]] bool is_ghost() const { return ghost; }
    [[nodiscard]] bool is_not_ghost() const { return !is_ghost(); }

    void update(bool g) { ghost = g; }

   private:
    bool ghost = false;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self.ghost                          //
        );
    }
};
