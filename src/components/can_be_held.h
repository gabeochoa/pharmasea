

#pragma once

#include "base_component.h"

struct CanBeHeld : public BaseComponent {
    [[nodiscard]] bool is_held() const { return held; }
    [[nodiscard]] bool is_not_held() const { return !is_held(); }

    void set_is_being_held(bool g) { held = g; }

   private:
    bool held = false;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.held                        //
        );
    }
};

struct CanBeHeld_HT : public CanBeHeld {};
