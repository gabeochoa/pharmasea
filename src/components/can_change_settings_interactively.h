
#pragma once

#include "base_component.h"

struct CanChangeSettingsInteractively : public BaseComponent {
    enum Style {
        Unknown,
        ToggleIsTutorial,
    } style = Unknown;

    CanChangeSettingsInteractively() : style(Unknown) {}
    explicit CanChangeSettingsInteractively(Style style) : style(style) {}

    virtual ~CanChangeSettingsInteractively() {}

   private:
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this), style);
    }
};

CEREAL_REGISTER_TYPE(CanChangeSettingsInteractively);
