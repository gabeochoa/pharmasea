#pragma once

#include <filesystem>

#include "../../ah.h"
#include "../../building_locations.h"
#include "../../components/has_subtype.h"
#include "../../components/is_round_settings_manager.h"
#include "../../components/is_trigger_area.h"
#include "../../components/simple_colored_box_renderer.h"
#include "../../entities/entity_helper.h"
#include "../../save_game/save_game.h"
#include "../ai/ai_tags.h"

namespace system_manager {

extern bool g_load_save_delete_mode;

struct UpdateDynamicTriggerAreaSettingsSystem
    : public afterhours::System<
          IsTriggerArea,
          afterhours::tags::All<
              afterhours::tags::TriggerTag::TriggerAreaHasDynamicText>> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsTriggerArea& ita,
                               float) override {
        if (ita.type == IsTriggerArea::Unset) {
            log_warn("You created a trigger area without a type");
            TranslatableString not_configured_ts =
                TODO_TRANSLATE("Not Configured", TodoReason::UserFacingError);
            ita.update_title(not_configured_ts);
            return;
        }

        switch (ita.type) {
            case IsTriggerArea::ModelTest_BackToLobby: {
                ita.update_title(NO_TRANSLATE("Back To Lobby"));
                ita.update_subtitle(TranslatableString(strings::i18n::LOADING));
                return;
            } break;
            case IsTriggerArea::Lobby_ModelTest: {
                ita.update_title(NO_TRANSLATE("Model Testing"));
                ita.update_subtitle(TranslatableString(strings::i18n::LOADING));
                return;
            } break;
            case IsTriggerArea::Lobby_PlayGame: {
                ita.update_title(TranslatableString(strings::i18n::START_GAME));
                ita.update_subtitle(TranslatableString(strings::i18n::LOADING));
                return;
            } break;
            case IsTriggerArea::Lobby_LoadSave: {
                ita.update_title(NO_TRANSLATE("Load / Save"));
                ita.update_subtitle(TranslatableString(strings::i18n::LOADING));
                return;
            } break;
            case IsTriggerArea::Store_BackToPlanning: {
                ita.update_title(TranslatableString(
                    strings::i18n::TRIGGERAREA_PURCHASE_FINISH));
                ita.update_subtitle(TranslatableString(
                    strings::i18n::TRIGGERAREA_PURCHASE_FINISH));
                return;
            } break;
            case IsTriggerArea::LoadSave_BackToLobby: {
                ita.update_title(NO_TRANSLATE("Back To Lobby"));
                ita.update_subtitle(TranslatableString(strings::i18n::LOADING));
                return;
            } break;
            case IsTriggerArea::LoadSave_ToggleDeleteMode: {
                const bool delete_mode = g_load_save_delete_mode;
                ita.update_title(delete_mode
                                     ? NO_TRANSLATE("Delete Mode: ON")
                                     : NO_TRANSLATE("Delete Mode: OFF"));
                ita.update_subtitle(TranslatableString(strings::i18n::LOADING));
                return;
            } break;
            case IsTriggerArea::Planning_SaveSlot: {
                int slot_num = entity.has<HasSubtype>()
                                   ? entity.get<HasSubtype>().get_type_index()
                                   : 1;
                if (slot_num < 1) slot_num = 1;

                const bool is_saving = ita.active_entrants() > 0 &&
                                       ita.progress() > 0.f &&
                                       ita.should_progress();
                if (is_saving) {
                    ita.update_title(NO_TRANSLATE("Saving..."));
                    ita.update_subtitle(
                        NO_TRANSLATE(fmt::format("Slot {:02d}", slot_num)));
                } else {
                    ita.update_title(
                        NO_TRANSLATE(fmt::format("Slot {:02d}", slot_num)));
                    ita.update_subtitle(NO_TRANSLATE("Save Game"));
                }
                return;
            } break;
            case IsTriggerArea::Store_Reroll: {
                Entity& sophie =
                    EntityHelper::getNamedEntity(NamedEntity::Sophie);
                const IsRoundSettingsManager& irsm =
                    sophie.get<IsRoundSettingsManager>();
                int reroll_cost = irsm.get<int>(ConfigKey::StoreRerollPrice);
                auto str =
                    TranslatableString(strings::i18n::StoreReroll)
                        .set_param(strings::i18nParam::RerollCost, reroll_cost);

                ita.update_title(str);
                ita.update_subtitle(str);
                return;
            } break;
            case IsTriggerArea::Unset:
            case IsTriggerArea::Progression_Option1:
            case IsTriggerArea::Progression_Option2:
                break;
            case IsTriggerArea::LoadSave_LoadSlot: {
                // Keep colors up to date:
                // - empty slots are grey
                // - loadable slots are green
                // - in delete mode, loadable slots are red (empty slots remain
                // grey)
                bool delete_mode = g_load_save_delete_mode;
                int slot_num = entity.has<HasSubtype>()
                                   ? entity.get<HasSubtype>().get_type_index()
                                   : 1;
                if (slot_num < 1) slot_num = 1;

                bool exists = std::filesystem::exists(
                    save_game::SaveGameManager::slot_path(slot_num));

                Color c = exists ? (delete_mode ? ui::color::red
                                                : ui::color::green_apple)
                                 : ui::color::grey;
                if (entity.has<SimpleColoredBoxRenderer>()) {
                    entity.get<SimpleColoredBoxRenderer>()
                        .update_face(c)
                        .update_base(c);
                }
                return;
            }
        }

        TranslatableString internal_ts =
            TODO_TRANSLATE("(internal)", TodoReason::UserFacingError);
        switch (ita.type) {
            case IsTriggerArea::Progression_Option1:  // fall through
            case IsTriggerArea::Progression_Option2: {
                ita.update_title(internal_ts);
                ita.update_subtitle(internal_ts);

                Entity& sophie =
                    EntityHelper::getNamedEntity(NamedEntity::Sophie);
                const IsProgressionManager& ipm =
                    sophie.get<IsProgressionManager>();

                if (!ipm.collectedOptions) {
                    ita.update_title(internal_ts);
                    ita.update_subtitle(internal_ts);
                    return;
                }

                bool isOption1 = ita.type == IsTriggerArea::Progression_Option1;

                ita.update_title(ipm.get_option_title(isOption1));
                ita.update_subtitle(ipm.get_option_subtitle(isOption1));
                return;
            } break;
            default:
                break;
        }

        TranslatableString not_configured_ts =
            TODO_TRANSLATE("Not Configured", TodoReason::UserFacingError);

        ita.update_title(not_configured_ts);
        ita.update_subtitle(not_configured_ts);
        log_warn(
            "Trying to update trigger area title but type {} not handled "
            "anywhere",
            ita.type);
    }
};

}  // namespace system_manager
