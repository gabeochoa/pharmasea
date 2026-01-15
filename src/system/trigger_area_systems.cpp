#include <filesystem>
#include <unordered_map>

#include "../ah.h"
#include "../building_locations.h"
#include "../client_server_comm.h"
#include "../components/has_subtype.h"
#include "../components/is_bank.h"
#include "../components/is_progression_manager.h"
#include "../components/is_round_settings_manager.h"
#include "../components/is_solid.h"
#include "../components/is_trigger_area.h"
#include "../components/simple_colored_box_renderer.h"
#include "../components/transform.h"
#include "../dataclass/ingredient.h"
#include "../engine/runtime_globals.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "../entity_query.h"
#include "../network/server.h"
#include "../save_game/save_game.h"
#include "ai_tags.h"
#include "progression.h"
#include "store_management_helpers.h"
#include "system_manager.h"
#include "trigger_area_systems.h"

namespace system_manager {

static bool g_load_save_delete_mode = false;

void generate_machines_for_new_upgrades();
void spawn_machines_for_newly_unlocked_drink_DONOTCALL(IsProgressionManager&,
                                                       Drink);
namespace progression {
void update_upgrade_variables();
}  // namespace progression
namespace store {
void cleanup_old_store_options();
void generate_store_options();
void move_purchased_furniture();
}  // namespace store

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
                ita.update_title(delete_mode ? NO_TRANSLATE("Delete Mode: ON")
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

                Color c =
                    exists ? (delete_mode ? ui::color::red
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

                bool isOption1 =
                    ita.type == IsTriggerArea::Progression_Option1;

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

struct CountAllPossibleTriggerAreaEntrantsSystem
    : public afterhours::System<IsTriggerArea> {
    int count = 0;

    virtual bool should_run(const float) override { return true; }

    virtual void once(float) override {
        count = static_cast<int>(EQ(SystemManager::get().oldAll)
                                     .whereType(EntityType::Player)
                                     .gen_count());
    }

    virtual void for_each_with(Entity&, IsTriggerArea& ita, float) override {
        ita.update_all_entrants(count);
    }
};

struct CountInBuildingTriggerAreaEntrantsSystem
    : public afterhours::System<IsTriggerArea> {
    std::unordered_map<BuildingType, int> counts;

    virtual bool should_run(const float) override { return true; }

    virtual void once(float) override {
        for (BuildingType type : magic_enum::enum_values<BuildingType>()) {
            const Building& b = get_building(type);
            counts[type] = static_cast<int>(EQ(SystemManager::get().oldAll)
                                                .whereType(EntityType::Player)
                                                .whereInside(b.min(), b.max())
                                                .gen_count());
        }
    }

    virtual void for_each_with(Entity&, IsTriggerArea& ita, float) override {
        if (!ita.building) return;
        ita.update_entrants_in_building(counts[ita.building->id]);
    }
};

struct CountTriggerAreaEntrantsSystem
    : public afterhours::System<IsTriggerArea> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsTriggerArea& ita,
                               float) override {
        size_t count =
            EQ(SystemManager::get().oldAll)
                .whereType(EntityType::Player)
                .whereCollides(
                    entity.get<Transform>().expanded_bounds({0, TILESIZE, 0}))
                .gen_count();

        ita.update_entrants(static_cast<int>(count));
    }
};

struct UpdateTriggerAreaPercentSystem
    : public afterhours::System<IsTriggerArea> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity&, IsTriggerArea& ita, float dt) override {
        ita.should_wave()  //
            ? ita.increase_progress(dt)
            : ita.decrease_progress(dt);

        if (!ita.should_progress()) ita.decrease_cooldown(dt);
    }
};

struct MarkTriggerAreaFullNeedsProcessingSystem
    : public afterhours::System<IsTriggerArea> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsTriggerArea& ita,
                               float) override {
        if (ita.progress() < 1.f) return;
        entity.enableTag(
            afterhours::tags::TriggerTag::TriggerAreaFullNeedsProcessing);
    }
};

struct TriggerCbOnFullProgressSystem
    : public afterhours::System<
          IsTriggerArea,
          afterhours::tags::All<
              afterhours::tags::TriggerTag::TriggerAreaFullNeedsProcessing>> {
    virtual bool should_run(const float) override { return true; }

    void handle_progress_full(Entity& entity, IsTriggerArea& ita) {
        if (entity.hasTag(
                afterhours::tags::TriggerTag::GateTriggerWhileOccupied) &&
            ita.active_entrants() > 0) {
            if (entity.hasTag(
                    afterhours::tags::TriggerTag::TriggerFiredWhileOccupied)) {
                return;
            }
            entity.enableTag(
                afterhours::tags::TriggerTag::TriggerFiredWhileOccupied);
        }

        ita.reset_cooldown();

        switch (ita.type) {
            case IsTriggerArea::Store_Reroll:
                handle_store_reroll();
                break;
            case IsTriggerArea::Unset:
                log_warn("Completed trigger area wait time but was Unset type");
                break;
            case IsTriggerArea::ModelTest_BackToLobby:
                handle_modeltest_back_to_lobby();
                break;
            case IsTriggerArea::Lobby_ModelTest:
                handle_lobby_modeltest();
                break;
            case IsTriggerArea::Lobby_LoadSave:
                handle_lobby_loadsave();
                break;

            case IsTriggerArea::Lobby_PlayGame:
                handle_lobby_playgame();
                break;
            case IsTriggerArea::Progression_Option1:
                choose_progression_option(0);
                break;
            case IsTriggerArea::Progression_Option2:
                choose_progression_option(1);
                break;
            case IsTriggerArea::Store_BackToPlanning:
                handle_store_back_to_planning();
                break;

            case IsTriggerArea::LoadSave_BackToLobby:
                handle_loadsave_back_to_lobby();
                break;

            case IsTriggerArea::LoadSave_LoadSlot:
                handle_loadsave_load_slot(entity);
                break;

            case IsTriggerArea::LoadSave_ToggleDeleteMode:
                handle_loadsave_toggle_delete_mode();
                break;

            case IsTriggerArea::Planning_SaveSlot:
                handle_planning_save_slot(entity);
                break;
        }
    }

    void handle_store_reroll() {
        system_manager::store::cleanup_old_store_options();
        system_manager::store::generate_store_options();
        OptEntity sophie =
            EntityQuery().whereType(EntityType::Sophie).gen_first();
        if (!sophie.valid()) return;

        IsBank& bank = sophie->get<IsBank>();
        IsRoundSettingsManager& irsm = sophie->get<IsRoundSettingsManager>();
        int reroll_price = irsm.get<int>(ConfigKey::StoreRerollPrice);
        bank.withdraw(reroll_price);
        irsm.config.permanently_modify(ConfigKey::StoreRerollPrice,
                                       Operation::Add, 25);
    }

    void move_all_players_to_state(game::State state) {
        for (RefEntity player : EQ(SystemManager::get().oldAll)
                                    .whereType(EntityType::Player)
                                    .gen()) {
            move_player_SERVER_ONLY(player, state);
        }
    }

    void move_players_in_building_to_state(const Building& building,
                                           game::State state) {
        for (RefEntity player : EQ(SystemManager::get().oldAll)
                                    .whereType(EntityType::Player)
                                    .whereInside(building.min(), building.max())
                                    .gen()) {
            move_player_SERVER_ONLY(player, state);
        }
    }

    void cleanup_entities_in_building(const Building& building,
                                      bool skip_trigger_areas) {
        const auto ents = EQ().getAllInRange(building.min(), building.max());
        for (Entity& to_delete : ents) {
            if (skip_trigger_areas && to_delete.has<IsTriggerArea>()) continue;
            to_delete.cleanup = true;
        }
    }

    void handle_modeltest_back_to_lobby() {
        GameState::get().transition_to_lobby();
        cleanup_entities_in_building(MODEL_TEST_BUILDING, true);
        move_all_players_to_state(game::State::Lobby);
    }

    void handle_lobby_modeltest() {
        GameState::get().transition_to_model_test();
        if (is_server()) {
            network::Server* server = globals::server();
            if (server) {
                server->get_map_SERVER_ONLY()->generate_model_test_map();
            }
        }
        move_all_players_to_state(game::State::ModelTest);
    }

    void handle_lobby_loadsave() {
        GameState::get().set(game::State::LoadSaveRoom);
        if (is_server()) {
            network::Server* server = globals::server();
            if (server) {
                server->get_map_SERVER_ONLY()->generate_load_save_room_map();
            }
        }
        move_all_players_to_state(game::State::LoadSaveRoom);
    }

    void handle_lobby_playgame() {
        GameState::get().transition_to_game();
        move_all_players_to_state(game::State::InGame);
    }

    void handle_store_back_to_planning() {
        system_manager::store::move_purchased_furniture();

        GameState::get().transition_to_game();

        move_players_in_building_to_state(STORE_BUILDING,
                                          game::State::InGame);
    }

    void handle_loadsave_back_to_lobby() {
        GameState::get().transition_to_lobby();
        cleanup_entities_in_building(LOAD_SAVE_BUILDING, false);
        move_all_players_to_state(game::State::Lobby);
    }

    void handle_loadsave_load_slot(Entity& entity) {
        int slot_num = loadsave_slot_num(entity);

        const bool delete_mode = g_load_save_delete_mode;
        if (delete_mode) {
            bool ok = server_only::delete_game_slot(slot_num);
            if (!ok) return;

            // Refresh room by clearing entities in the building and regenerating.
            cleanup_entities_in_building(LOAD_SAVE_BUILDING, false);
            network::Server* server = globals::server();
            if (server) {
                server->get_map_SERVER_ONLY()->generate_load_save_room_map();
                server->force_send_map_state();
            }
            return;
        }

        bool ok = server_only::load_game_from_slot(slot_num);
        if (!ok) return;

        // Always land in planning (InGame).
        GameState::get().transition_to_game();
        move_all_players_to_state(game::State::InGame);
    }

    void handle_loadsave_toggle_delete_mode() {
        g_load_save_delete_mode = !g_load_save_delete_mode;
        for (RefEntity trigger_area :
             EntityQuery()
                 .whereTriggerAreaOfType(IsTriggerArea::LoadSave_LoadSlot)
                 .gen()) {
            Entity& entity = trigger_area.get();
            if (g_load_save_delete_mode) {
                entity.enableTag(
                    afterhours::tags::TriggerTag::GateTriggerWhileOccupied);
            } else {
                entity.disableTag(
                    afterhours::tags::TriggerTag::GateTriggerWhileOccupied);
            }
            entity.disableTag(
                afterhours::tags::TriggerTag::TriggerFiredWhileOccupied);
        }
        network::Server::forward_packet(network::ClientPacket{
            .channel = Channel::RELIABLE,
            .client_id = network::SERVER_CLIENT_ID,
            .msg_type = network::ClientPacket::MsgType::Announcement,
            .msg =
                network::ClientPacket::AnnouncementInfo{
                    .message = g_load_save_delete_mode ? "Delete mode ON"
                                                       : "Delete mode OFF",
                    .type = g_load_save_delete_mode
                                ? AnnouncementType::Warning
                                : AnnouncementType::Message,
                },
        });
    }

    void handle_planning_save_slot(Entity& entity) {
        if (SystemManager::get().is_bar_closed()) {
            network::Server::forward_packet(network::ClientPacket{
                .channel = Channel::RELIABLE,
                .client_id = network::SERVER_CLIENT_ID,
                .msg_type = network::ClientPacket::MsgType::Announcement,
                .msg =
                    network::ClientPacket::AnnouncementInfo{
                        .message =
                            "Can't save right now (planning/daytime only).",
                        .type = AnnouncementType::Warning,
                    },
            });
            return;
        }
        int slot_num = loadsave_slot_num(entity);
        bool ok = server_only::save_game_to_slot(slot_num);
        network::Server::forward_packet(network::ClientPacket{
            .channel = Channel::RELIABLE,
            .client_id = network::SERVER_CLIENT_ID,
            .msg_type = network::ClientPacket::MsgType::Announcement,
            .msg =
                network::ClientPacket::AnnouncementInfo{
                    .message =
                        ok ? fmt::format("Saved to slot {:02d}", slot_num)
                           : fmt::format("Failed to save slot {:02d}", slot_num),
                    .type = ok ? AnnouncementType::Message
                               : AnnouncementType::Error,
                },
        });
    }

    int loadsave_slot_num(Entity& entity) const {
        int slot_num = entity.has<HasSubtype>()
                           ? entity.get<HasSubtype>().get_type_index()
                           : 1;
        if (slot_num < 1) slot_num = 1;
        return slot_num;
    }

    void choose_progression_option(int option_chosen) {
        for (RefEntity door :
             EntityQuery()
                 .whereType(EntityType::Door)
                 .whereInside(PROGRESSION_BUILDING.min(),
                              PROGRESSION_BUILDING.max())
                 .gen()) {
            door.get().addComponentIfMissing<IsSolid>();
        }

        GameState::get().transition_to_game();

        move_players_in_building_to_state(PROGRESSION_BUILDING,
                                          game::State::InGame);

        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        IsProgressionManager& ipm = sophie.get<IsProgressionManager>();
        IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

        switch (ipm.upgrade_type()) {
            case UpgradeType::None:
                break;
            case UpgradeType::Upgrade: {
                const UpgradeClass& option =
                    (option_chosen == 0 ? ipm.upgradeOption1
                                        : ipm.upgradeOption2);
                auto optionImpl = make_upgrade(option);
                optionImpl->onUnlock(irsm.config, ipm);
                irsm.selected_upgrades.push_back(optionImpl);
                irsm.config.mark_upgrade_unlocked(option);
                generate_machines_for_new_upgrades();
            } break;
            case UpgradeType::Drink: {
                Drink option =
                    option_chosen == 0 ? ipm.drinkOption1 : ipm.drinkOption2;

                ipm.unlock_drink(option);
                auto possibleNewIGs = get_req_ingredients_for_drink(option);
                bitset_utils::for_each_enabled_bit(
                    possibleNewIGs, [&ipm](size_t index) {
                        Ingredient ig =
                            magic_enum::enum_value<Ingredient>(index);
                        ipm.unlock_ingredient(ig);
                        return bitset_utils::ForEachFlow::NormalFlow;
                    });

                spawn_machines_for_newly_unlocked_drink_DONOTCALL(ipm, option);
            } break;
        }

        ipm.next_round();
        system_manager::progression::update_upgrade_variables();
    }

    virtual void for_each_with(Entity& entity, IsTriggerArea& ita,
                               float) override {
        handle_progress_full(entity, ita);
        entity.disableTag(
            afterhours::tags::TriggerTag::TriggerAreaFullNeedsProcessing);
    }
};

struct ResetTriggerFiredWhileOccupiedSystem
    : public afterhours::System<
          IsTriggerArea,
          afterhours::tags::All<
              afterhours::tags::TriggerTag::GateTriggerWhileOccupied>> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsTriggerArea& ita,
                               float) override {
        if (ita.active_entrants() > 0) return;
        entity.disableTag(
            afterhours::tags::TriggerTag::TriggerFiredWhileOccupied);
    }
};

void register_trigger_area_systems(afterhours::SystemManager& systems) {
    systems.register_update_system(
        std::make_unique<system_manager::UpdateDynamicTriggerAreaSettingsSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::CountAllPossibleTriggerAreaEntrantsSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::CountInBuildingTriggerAreaEntrantsSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::CountTriggerAreaEntrantsSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::UpdateTriggerAreaPercentSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::ResetTriggerFiredWhileOccupiedSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::MarkTriggerAreaFullNeedsProcessingSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::TriggerCbOnFullProgressSystem>());
}

}  // namespace system_manager

