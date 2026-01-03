
#pragma once

#include "base_component.h"

struct CanBeTakenFrom : public BaseComponent {
    [[nodiscard]] bool can_take_from() const { return allowed; }
    [[nodiscard]] bool cannot_take_from() const { return !can_take_from(); }

    void update(bool g) { allowed = g; }

   private:
    bool allowed = false;

    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.allowed                     //
        );
    }
};
