#pragma once

#include "../../ah.h"
#include "../../components/can_change_settings_interactively.h"
#include "../../components/has_name.h"
#include "../../components/is_round_settings_manager.h"
#include "../../components/simple_colored_box_renderer.h"
#include "../../entity_helper.h"
#include "../../external_include.h"

namespace system_manager {

struct UpdateVisualsForSettingsChangerSystem
    : public afterhours::System<CanChangeSettingsInteractively> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, CanChangeSettingsInteractively&,
                               float) override {
        // TODO we can probabyl also filter on IRSM
        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

        auto get_name = [](CanChangeSettingsInteractively::Style style,
                           bool bool_value) {
            auto bool_str = bool_value ? "Enabled" : "Disabled";

            switch (style) {
                case CanChangeSettingsInteractively::ToggleIsTutorial:
                    return fmt::format("Tutorial: {}", bool_str);
                    break;
                case CanChangeSettingsInteractively::Unknown:
                    break;
            }
            log_warn(
                "Tried to get_name for Interactive Setting Style {} but it was "
                "not "
                "handled",
                magic_enum::enum_name<CanChangeSettingsInteractively::Style>(
                    style));
            return std::string("");
        };

        auto update_color_for_bool = [](Entity& isc, bool value) {
            // TODO eventually read from theme... (imports)
            // auto color = value ? UI_THEME.from_usage(theme::Usage::Primary)
            // : UI_THEME.from_usage(theme::Usage::Error);

            auto color = value ?  //
                             ::ui::color::green_apple
                               : ::ui::color::red;

            isc.get<SimpleColoredBoxRenderer>()  //
                .update_face(color)
                .update_base(color);
        };

        auto style = entity.get<CanChangeSettingsInteractively>().style;

        switch (style) {
            case CanChangeSettingsInteractively::ToggleIsTutorial: {
                entity.get<HasName>().update(get_name(
                    style, irsm.interactive_settings.is_tutorial_active));
                update_color_for_bool(
                    entity, irsm.interactive_settings.is_tutorial_active);
            } break;
            case CanChangeSettingsInteractively::Unknown:
                break;
        }
    }
};

}  // namespace system_manager
