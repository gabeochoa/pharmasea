
#pragma once

#include "base_component.h"

struct CanChangeSettingsInteractively : public BaseComponent {
    enum Style {
        Unknown,
        ToggleIsTutorial,
    } style = Unknown;

    CanChangeSettingsInteractively() : style(Unknown) {}
    explicit CanChangeSettingsInteractively(Style style) : style(style) {}

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.style                      //
        );
    }
};
