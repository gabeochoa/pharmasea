
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
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(style);
    }
};
