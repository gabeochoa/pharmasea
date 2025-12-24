#include "../ah.h"
#include "../building_locations.h"
#include "../components/can_be_highlighted.h"
#include "../components/can_change_settings_interactively.h"
#include "../components/can_highlight_others.h"
#include "../components/can_hold_handtruck.h"
#include "../components/can_hold_item.h"
#include "../components/can_order_drink.h"
#include "../components/conveys_held_item.h"
#include "../components/custom_item_position.h"
#include "../components/has_dynamic_model_name.h"
#include "../components/has_name.h"
#include "../components/is_bank.h"
#include "../components/is_drink.h"
#include "../components/is_floor_marker.h"
#include "../components/is_nux_manager.h"
#include "../components/is_pnumatic_pipe.h"
#include "../components/is_round_settings_manager.h"
#include "../components/is_snappable.h"
#include "../components/is_solid.h"
#include "../components/is_squirter.h"
#include "../components/is_trigger_area.h"
#include "../components/has_subtype.h"
#include "../components/model_renderer.h"
#include "../components/simple_colored_box_renderer.h"
#include "../components/transform.h"
#include "../components/uses_character_model.h"
#include "../dataclass/ingredient.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "../entity_makers.h"
#include "../entity_query.h"
#include "../external_include.h"
#include "../network/server.h"
#include "../client_server_comm.h"
#include "../vec_util.h"
#include "progression.h"
#include "store_management_helpers.h"
#include "system_manager.h"

namespace system_manager {

static constexpr const char* kLoadSaveDeleteModeKey = "load_save_delete_mode";

bool _create_nuxes(Entity& entity);
void process_nux_updates(Entity& entity, float dt);
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
void update_dynamic_trigger_area_settings(Entity& entity, float dt);
void count_all_possible_trigger_area_entrants(Entity& entity, float dt);
void count_in_building_trigger_area_entrants(Entity& entity, float dt);
void count_trigger_area_entrants(Entity& entity, float dt);
void update_trigger_area_percent(Entity& entity, float dt);
void trigger_cb_on_full_progress(Entity& entity, float dt);

void count_all_possible_trigger_area_entrants(Entity& entity, float) {
    if (entity.is_missing<IsTriggerArea>()) return;

    size_t count = EQ(SystemManager::get().oldAll)
                       .whereType(EntityType::Player)
                       .gen_count();

    entity.get<IsTriggerArea>().update_all_entrants(static_cast<int>(count));
}

void count_trigger_area_entrants(Entity& entity, float) {
    if (entity.is_missing<IsTriggerArea>()) return;

    size_t count = EQ(SystemManager::get().oldAll)
                       .whereType(EntityType::Player)
                       .whereCollides(entity.get<Transform>().expanded_bounds(
                           {0, TILESIZE, 0}))
                       .gen_count();

    entity.get<IsTriggerArea>().update_entrants(static_cast<int>(count));
}

void count_in_building_trigger_area_entrants(Entity& entity, float) {
    if (entity.is_missing<IsTriggerArea>()) return;

    std::optional<Building> building = entity.get<IsTriggerArea>().building;
    if (!building) return;

    size_t count =
        EQ(SystemManager::get().oldAll)
            .whereType(EntityType::Player)
            .whereInside(building.value().min(), building.value().max())
            .gen_count();

    entity.get<IsTriggerArea>().update_entrants_in_building(
        static_cast<int>(count));
}

void update_trigger_area_percent(Entity& entity, float dt) {
    if (entity.is_missing<IsTriggerArea>()) return;
    IsTriggerArea& ita = entity.get<IsTriggerArea>();

    ita.should_wave()  //
        ? ita.increase_progress(dt)
        : ita.decrease_progress(dt);

    if (!ita.should_progress()) ita.decrease_cooldown(dt);
}

void trigger_cb_on_full_progress(Entity& entity, float) {
    if (entity.is_missing<IsTriggerArea>()) return;
    IsTriggerArea& ita = entity.get<IsTriggerArea>();
    if (ita.progress() < 1.f) return;

    ita.reset_cooldown();

    const auto _choose_option = [](int option_chosen) {
        for (RefEntity door : EntityQuery()
                                  .whereType(EntityType::Door)
                                  .whereInside(PROGRESSION_BUILDING.min(),
                                               PROGRESSION_BUILDING.max())
                                  .gen()) {
            door.get().addComponentIfMissing<IsSolid>();
        }

        GameState::get().transition_to_game();

        for (RefEntity player : EQ(SystemManager::get().oldAll)
                                    .whereType(EntityType::Player)
                                    .whereInside(PROGRESSION_BUILDING.min(),
                                                 PROGRESSION_BUILDING.max())
                                    .gen()) {
            move_player_SERVER_ONLY(player, game::State::InGame);
        }

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
    };

    switch (ita.type) {
        case IsTriggerArea::Store_Reroll: {
            system_manager::store::cleanup_old_store_options();
            system_manager::store::generate_store_options();
            OptEntity sophie =
                EntityQuery().whereType(EntityType::Sophie).gen_first();
            if (sophie.valid()) {
                IsBank& bank = sophie->get<IsBank>();
                IsRoundSettingsManager& irsm =
                    sophie->get<IsRoundSettingsManager>();
                int reroll_price = irsm.get<int>(ConfigKey::StoreRerollPrice);
                bank.withdraw(reroll_price);
                irsm.config.permanently_modify(ConfigKey::StoreRerollPrice,
                                               Operation::Add, 25);
            }

        } break;
        case IsTriggerArea::Unset:
            log_warn("Completed trigger area wait time but was Unset type");
            break;
        case IsTriggerArea::ModelTest_BackToLobby: {
            GameState::get().transition_to_lobby();

            const auto ents = EntityHelper::getAllInRange(
                MODEL_TEST_BUILDING.min(), MODEL_TEST_BUILDING.max());

            for (Entity& to_delete : ents) {
                if (to_delete.has<IsTriggerArea>()) continue;
                to_delete.cleanup = true;
            }

            for (RefEntity player : EQ(SystemManager::get().oldAll)
                                        .whereType(EntityType::Player)
                                        .gen()) {
                move_player_SERVER_ONLY(player, game::State::Lobby);
            }
        } break;
        case IsTriggerArea::Lobby_ModelTest: {
            GameState::get().transition_to_model_test();
            if (is_server()) {
                network::Server* server =
                    GLOBALS.get_ptr<network::Server>("server");
                server->get_map_SERVER_ONLY()
                    ->game_info.generate_model_test_map();
            }

            for (RefEntity player : EQ(SystemManager::get().oldAll)
                                        .whereType(EntityType::Player)
                                        .gen()) {
                move_player_SERVER_ONLY(player, game::State::ModelTest);
            }
        } break;
        case IsTriggerArea::Lobby_LoadSave: {
            GameState::get().set(game::State::LoadSaveRoom);
            if (is_server()) {
                network::Server* server =
                    GLOBALS.get_ptr<network::Server>("server");
                server->get_map_SERVER_ONLY()
                    ->game_info.generate_load_save_room_map();
            }
            for (RefEntity player : EQ(SystemManager::get().oldAll)
                                        .whereType(EntityType::Player)
                                        .gen()) {
                move_player_SERVER_ONLY(player, game::State::LoadSaveRoom);
            }
        } break;

        case IsTriggerArea::Lobby_PlayGame: {
            GameState::get().transition_to_game();
            for (RefEntity player : EQ(SystemManager::get().oldAll)
                                        .whereType(EntityType::Player)
                                        .gen()) {
                move_player_SERVER_ONLY(player, game::State::InGame);
            }
        } break;
        case IsTriggerArea::Progression_Option1:
            _choose_option(0);
            break;
        case IsTriggerArea::Progression_Option2:
            _choose_option(1);
            break;
        case IsTriggerArea::Store_BackToPlanning: {
            system_manager::store::move_purchased_furniture();

            GameState::get().transition_to_game();

            for (RefEntity player :
                 EQ(SystemManager::get().oldAll)
                     .whereType(EntityType::Player)
                     .whereInside(STORE_BUILDING.min(), STORE_BUILDING.max())
                     .gen()) {
                move_player_SERVER_ONLY(player, game::State::InGame);
            }
        } break;

        case IsTriggerArea::LoadSave_BackToLobby: {
            GameState::get().transition_to_lobby();

            const auto ents = EntityHelper::getAllInRange(
                LOAD_SAVE_BUILDING.min(), LOAD_SAVE_BUILDING.max());
            for (Entity& to_delete : ents) {
                to_delete.cleanup = true;
            }

            for (RefEntity player : EQ(SystemManager::get().oldAll)
                                        .whereType(EntityType::Player)
                                        .gen()) {
                move_player_SERVER_ONLY(player, game::State::Lobby);
            }
        } break;

        case IsTriggerArea::LoadSave_LoadSlot: {
            int slot_num = entity.has<HasSubtype>()
                               ? entity.get<HasSubtype>().get_type_index()
                               : 1;
            if (slot_num < 1) slot_num = 1;

            const bool delete_mode =
                GLOBALS.get_or_default<bool>(kLoadSaveDeleteModeKey, false);
            if (delete_mode) {
                bool ok = server_only::delete_game_slot(slot_num);
                if (!ok) break;

                // Refresh room by clearing entities in the building and regenerating.
                const auto ents = EntityHelper::getAllInRange(
                    LOAD_SAVE_BUILDING.min(), LOAD_SAVE_BUILDING.max());
                for (Entity& to_delete : ents) {
                    to_delete.cleanup = true;
                }
                network::Server* server =
                    GLOBALS.get_ptr<network::Server>("server");
                if (server) {
                    server->get_map_SERVER_ONLY()
                        ->game_info.generate_load_save_room_map();
                    server->force_send_map_state();
                }
                break;
            }

            bool ok = server_only::load_game_from_slot(slot_num);
            if (!ok) break;

            // Always land in planning (InGame).
            GameState::get().transition_to_game();
            for (RefEntity player : EQ(SystemManager::get().oldAll)
                                        .whereType(EntityType::Player)
                                        .gen()) {
                move_player_SERVER_ONLY(player, game::State::InGame);
            }
        } break;

        case IsTriggerArea::LoadSave_ToggleDeleteMode: {
            bool current =
                GLOBALS.get_or_default<bool>(kLoadSaveDeleteModeKey, false);
            GLOBALS.set(kLoadSaveDeleteModeKey, !current);
        } break;

        case IsTriggerArea::Planning_SaveSlot: {
            if (!SystemManager::get().is_daytime()) break;
            int slot_num = entity.has<HasSubtype>()
                               ? entity.get<HasSubtype>().get_type_index()
                               : 1;
            if (slot_num < 1) slot_num = 1;
            (void) server_only::save_game_to_slot(slot_num);
        } break;
    }
}

void update_dynamic_trigger_area_settings(Entity& entity, float) {
    if (entity.is_missing<IsTriggerArea>()) return;
    IsTriggerArea& ita = entity.get<IsTriggerArea>();

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
            ita.update_title(
                TranslatableString(strings::i18n::TRIGGERAREA_PURCHASE_FINISH));
            ita.update_subtitle(
                TranslatableString(strings::i18n::TRIGGERAREA_PURCHASE_FINISH));
            return;
        } break;
        case IsTriggerArea::LoadSave_BackToLobby: {
            ita.update_title(NO_TRANSLATE("Back To Lobby"));
            ita.update_subtitle(TranslatableString(strings::i18n::LOADING));
            return;
        } break;
        case IsTriggerArea::LoadSave_ToggleDeleteMode: {
            const bool delete_mode =
                GLOBALS.get_or_default<bool>(kLoadSaveDeleteModeKey, false);
            ita.update_title(delete_mode ? NO_TRANSLATE("Delete Mode: ON")
                                         : NO_TRANSLATE("Delete Mode: OFF"));
            ita.update_subtitle(TranslatableString(strings::i18n::LOADING));
            return;
        } break;
        case IsTriggerArea::Planning_SaveSlot: {
            // If you'd like slot-specific text, set it at spawn-time.
            ita.update_title(NO_TRANSLATE("Save Game"));
            ita.update_subtitle(TranslatableString(strings::i18n::LOADING));
            return;
        } break;
        case IsTriggerArea::Store_Reroll: {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
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
        case IsTriggerArea::LoadSave_LoadSlot:
            return;
    }

    TranslatableString internal_ts =
        TODO_TRANSLATE("(internal)", TodoReason::UserFacingError);
    switch (ita.type) {
        case IsTriggerArea::Progression_Option1:  // fall through
        case IsTriggerArea::Progression_Option2: {
            ita.update_title(internal_ts);
            ita.update_subtitle(internal_ts);

            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
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

vec3 get_new_held_position_custom(Entity& entity) {
    const Transform& transform = entity.get<Transform>();
    vec3 new_pos = transform.pos();

    const CustomHeldItemPosition& custom_item_position =
        entity.get<CustomHeldItemPosition>();

    switch (custom_item_position.positioner) {
        case CustomHeldItemPosition::Positioner::Table:
            if (entity.has<ModelRenderer>()) {
                new_pos.y += TILESIZE / 2.f;
            }
            new_pos.y += 0.f;
            break;
        case CustomHeldItemPosition::Positioner::ItemHoldingItem:
            new_pos.x += 0;
            new_pos.y += 0;
            break;
        case CustomHeldItemPosition::Positioner::Blender:
            new_pos.x += 0;
            new_pos.y += TILESIZE * 2.f;
            break;
        case CustomHeldItemPosition::Positioner::Conveyer: {
            if (entity.is_missing<ConveysHeldItem>()) {
                log_warn("A conveyer positioned item needs ConveysHeldItem");
                break;
            }
            const ConveysHeldItem& conveysHeldItem =
                entity.get<ConveysHeldItem>();
            if (transform.face_direction() &
                Transform::FrontFaceDirection::FORWARD) {
                new_pos.z += TILESIZE * conveysHeldItem.relative_item_pos;
            }
            if (transform.face_direction() &
                Transform::FrontFaceDirection::RIGHT) {
                new_pos.x += TILESIZE * conveysHeldItem.relative_item_pos;
            }
            if (transform.face_direction() &
                Transform::FrontFaceDirection::BACK) {
                new_pos.z -= TILESIZE * conveysHeldItem.relative_item_pos;
            }
            if (transform.face_direction() &
                Transform::FrontFaceDirection::LEFT) {
                new_pos.x -= TILESIZE * conveysHeldItem.relative_item_pos;
            }
            new_pos.y += TILESIZE / 4;
        } break;
        case CustomHeldItemPosition::Positioner::PnumaticPipe: {
            if (entity.is_missing<IsPnumaticPipe>()) {
                log_warn("pipe positioned item needs ispnumaticpipe");
                break;
            }
            if (entity.is_missing<ConveysHeldItem>()) {
                log_warn("pipe positioned item needs ConveysHeldItem");
                break;
            }
            const ConveysHeldItem& conveysHeldItem =
                entity.get<ConveysHeldItem>();
            int mult = entity.get<IsPnumaticPipe>().recieving ? 1 : -1;
            new_pos.y += mult * TILESIZE * conveysHeldItem.relative_item_pos;
        } break;
    }
    return new_pos;
}

vec3 get_new_held_position_default(Entity& entity) {
    const Transform& transform = entity.get<Transform>();
    vec3 new_pos = transform.pos();
    if (transform.face_direction() & Transform::FrontFaceDirection::FORWARD) {
        new_pos.z += TILESIZE;
    }
    if (transform.face_direction() & Transform::FrontFaceDirection::RIGHT) {
        new_pos.x += TILESIZE;
    }
    if (transform.face_direction() & Transform::FrontFaceDirection::BACK) {
        new_pos.z -= TILESIZE;
    }
    if (transform.face_direction() & Transform::FrontFaceDirection::LEFT) {
        new_pos.x -= TILESIZE;
    }
    return new_pos;
}

struct DeleteCustomersWhenLeavingInroundSystem
    : public afterhours::System<CanOrderDrink, Transform> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, CanOrderDrink&,
                               Transform& transform, float) override {
        if (!check_type(entity, EntityType::Customer)) return;

        if (vec::distance(transform.as2(), {GATHER_SPOT, GATHER_SPOT}) > 2.f)
            return;
        entity.cleanup = true;
    }
};

struct HighlightFacingFurnitureSystem
    : public afterhours::System<CanHighlightOthers, Transform> {
    virtual void for_each_with(Entity& entity, CanHighlightOthers& cho,
                               Transform& transform, float) override {
        OptEntity match = EntityQuery()
                              .whereHasComponent<CanBeHighlighted>()
                              .whereInRange(transform.as2(), cho.reach())
                              .include_store_entities()
                              .orderByDist(transform.as2())
                              .gen_first();
        if (!match) return;

        match->get<CanBeHighlighted>().update(entity, true);
    }
};

struct ClearAllFloorMarkersSystem : public afterhours::System<IsFloorMarker> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsFloorMarker& ifm,
                               float) override {
        if (!check_type(entity, EntityType::FloorMarker)) return;
        ifm.clear();
    }
};

struct MarkItemInFloorAreaSystem
    : public afterhours::System<IsFloorMarker, Transform> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsFloorMarker& ifm,
                               Transform& transform, float) override {
        if (!check_type(entity, EntityType::FloorMarker)) return;

        std::vector<int> ids =
            // TODO do we need to do this now or can we use merge_temp
            EQ(SystemManager::get().oldAll)
                .whereNotID(entity.id)  // not us
                .whereNotType(EntityType::Player)
                .whereNotType(EntityType::RemotePlayer)
                .whereNotType(EntityType::SodaSpout)
                .whereHasComponent<IsSolid>()
                .whereCollides(transform.expanded_bounds({0, TILESIZE, 0}))
                // we want to include store items since it has many floor areas
                .include_store_entities()
                .gen_ids();

        ifm.mark_all(std::move(ids));
    }
};

struct ProcessNuxUpdatesSystem
    : public afterhours::System<IsNuxManager, IsRoundSettingsManager> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsNuxManager& inm,
                               IsRoundSettingsManager&, float dt) override {
        // Tutorial isnt on so dont do any nuxes
        if (!entity.get<IsRoundSettingsManager>()
                 .interactive_settings.is_tutorial_active) {
            return;
        }

        // only generate the nux once you leave the lobby
        if (!GameState::get().is_game_like()) return;

        if (!inm.initialized) {
            bool init = _create_nuxes(entity);
            if (!init) return;
            inm.initialized = init;
        }

        // Forward to full implementation
        process_nux_updates(entity, dt);
    }
};

struct ProcessSodaFountainSystem
    : public afterhours::System<
          CanHoldItem, afterhours::tags::All<EntityType::SodaFountain>> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity&, CanHoldItem& sfCHI, float) override {
        // If we arent holding anything, nothing to squirt into
        if (sfCHI.empty()) return;

        if (sfCHI.item().is_missing<IsDrink>()) return;

        Entity& drink = sfCHI.item();
        // Already has soda in it
        if (bitset_utils::test(drink.get<IsDrink>().ing(), Ingredient::Soda)) {
            return;
        }

        items::_add_ingredient_to_drink_NO_VALIDATION(drink, Ingredient::Soda);
    }
};

struct ProcessSquirterSystem
    : public afterhours::System<IsSquirter, CanHoldItem, Transform> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with([[maybe_unused]] Entity& entity,
                               IsSquirter& is_squirter, CanHoldItem& sqCHI,
                               Transform& transform, float dt) override {
        // If we arent holding anything, nothing to squirt into
        if (sqCHI.empty()) {
            is_squirter.reset();
            is_squirter.set_drink_id(-1);
            return;
        }

        // cant squirt into this !
        if (sqCHI.item().is_missing<IsDrink>()) return;

        // so we got something, lets see if anyone around can give us
        // something to use

        auto pos = transform.as2();
        OptEntity closest_furniture =
            EntityQuery()
                .whereHasComponentAndLambda<CanHoldItem>(
                    [](const CanHoldItem& chi) {
                        if (chi.empty()) return false;
                        const Item& item = chi.const_item();
                        // TODO should we instead check for <AddsIngredient>?
                        if (!check_type(item, EntityType::Alcohol))
                            return false;
                        return true;
                    })
                .whereInRange(pos, 1.25f)
                // NOTE: if you change this make sure that this always sorts the
                // same per game version
                .orderByDist(pos)
                .gen_first();

        if (!closest_furniture) {
            // Nothing anymore, probably someone took the item away
            // working reset
            return;
        }
        Entity& drink = sqCHI.item();
        Item& item = closest_furniture->get<CanHoldItem>().item();

        if (is_squirter.drink_id() == drink.id) {
            is_squirter.reset();
            return;
        }

        if (is_squirter.item_id() == -1) {
            is_squirter.update(item.id,
                               closest_furniture->get<Transform>().pos());
        }
        vec3 item_position = is_squirter.picked_up_at();

        if (item.id != is_squirter.item_id()) {
            log_warn("squirter : item changed while working was {} now {}",
                     is_squirter.item_id(), item.id);
            is_squirter.reset();
            return;
        }

        if (vec::distance(vec::to2(item_position),
                          closest_furniture->get<Transform>().as2()) > 1.f) {
            log_warn("squirter : we matched something different {} {}",
                     item_position, closest_furniture->get<Transform>().as2());
            // We matched something different
            is_squirter.reset();
            return;
        }

        bool complete = is_squirter.pass_time(dt);
        if (!complete) {
            // keep going :)
            return;
        }

        is_squirter.set_drink_id(drink.id);

        bool cleanup = items::_add_item_to_drink_NO_VALIDATION(drink, item);
        if (cleanup) {
            closest_furniture->get<CanHoldItem>().update(nullptr, -1);
        }
    }
};

struct ProcessTrashSystem : public afterhours::System<CanHoldItem> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, CanHoldItem& trashCHI,
                               float) override {
        if (!check_type(entity, EntityType::Trash)) return;

        // If we arent holding anything, nothing to delete
        if (trashCHI.empty()) return;

        trashCHI.item().cleanup = true;
        trashCHI.update(nullptr, -1);
    }
};

struct UpdateDynamicTriggerAreaSettingsSystem
    : public afterhours::System<IsTriggerArea> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsTriggerArea&,
                               float dt) override {
        update_dynamic_trigger_area_settings(entity, dt);
    }
};

struct CountAllPossibleTriggerAreaEntrantsSystem
    : public afterhours::System<IsTriggerArea> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsTriggerArea&,
                               float dt) override {
        count_all_possible_trigger_area_entrants(entity, dt);
    }
};

struct CountInBuildingTriggerAreaEntrantsSystem
    : public afterhours::System<IsTriggerArea> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsTriggerArea&,
                               float dt) override {
        count_in_building_trigger_area_entrants(entity, dt);
    }
};

struct CountTriggerAreaEntrantsSystem
    : public afterhours::System<IsTriggerArea> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsTriggerArea&,
                               float dt) override {
        count_trigger_area_entrants(entity, dt);
    }
};

struct UpdateTriggerAreaPercentSystem
    : public afterhours::System<IsTriggerArea> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsTriggerArea&,
                               float dt) override {
        update_trigger_area_percent(entity, dt);
    }
};

struct TriggerCbOnFullProgressSystem
    : public afterhours::System<IsTriggerArea> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsTriggerArea&,
                               float dt) override {
        trigger_cb_on_full_progress(entity, dt);
    }
};

struct RefetchDynamicModelNamesSystem
    : public afterhours::System<HasDynamicModelName, ModelRenderer> {
    virtual void for_each_with(Entity& entity, HasDynamicModelName& hDMN,
                               ModelRenderer& renderer, float) override {
        renderer.update_model_name(hDMN.fetch(entity));
    }
};

struct ResetHighlightedSystem : public afterhours::System<CanBeHighlighted> {
    virtual void for_each_with(Entity& entity, CanBeHighlighted& cbh,
                               float) override {
        cbh.update(entity, false);
    }
};

struct TransformSnapperSystem : public afterhours::System<Transform> {
    virtual void for_each_with(Entity& entity, Transform& transform,
                               float) override {
        transform.update(entity.has<IsSnappable>()  //
                             ? transform.snap_position()
                             : transform.raw());
    }
};

struct UpdateCharacterModelFromIndexSystem
    : public afterhours::System<UsesCharacterModel, ModelRenderer> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with([[maybe_unused]] Entity& entity,
                               UsesCharacterModel& usesCharacterModel,
                               ModelRenderer& renderer, float) override {
        if (usesCharacterModel.value_same_as_last_render()) return;

        // TODO this should be the same as all other rendere updates for players
        renderer.update_model_name(usesCharacterModel.fetch_model_name());
        usesCharacterModel.mark_change_completed();
    }
};

struct UpdateHeldHandTruckPositionSystem
    : public afterhours::System<CanHoldHandTruck, Transform> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity&, CanHoldHandTruck& can_hold_hand_truck,
                               Transform& transform, float) override {
        if (can_hold_hand_truck.empty()) return;

        auto new_pos = transform.pos();

        OptEntity hand_truck =
            EntityHelper::getEntityForID(can_hold_hand_truck.hand_truck_id());
        hand_truck->get<Transform>().update(new_pos);
    }
};

struct UpdateHeldItemPositionSystem
    : public afterhours::System<CanHoldItem, Transform> {
    virtual void for_each_with(Entity& entity, CanHoldItem& can_hold_item,
                               Transform&, float) override {
        if (can_hold_item.empty()) return;

        vec3 new_pos = entity.has<CustomHeldItemPosition>()
                           ? get_new_held_position_custom(entity)
                           : get_new_held_position_default(entity);

        can_hold_item.item().get<Transform>().update(new_pos);
    }
};

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

void SystemManager::register_sixtyfps_systems() {
    // This system should run in all states (lobby, game, model test, etc.)
    // because it handles trigger areas and other essential updates
    // Note: We run every frame for better responsiveness (especially for
    // trigger areas), which is acceptable since these operations are
    // lightweight. The original ran at 60fps (when timePassed >= 0.016f), but
    // running every frame ensures trigger areas and other interactions feel
    // more responsive.

    systems.register_update_system(
        std::make_unique<system_manager::ClearAllFloorMarkersSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::MarkItemInFloorAreaSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::UpdateDynamicTriggerAreaSettingsSystem>());
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
        std::make_unique<system_manager::TriggerCbOnFullProgressSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessNuxUpdatesSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::UpdateCharacterModelFromIndexSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessSodaFountainSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessTrashSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::DeleteCustomersWhenLeavingInroundSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::TransformSnapperSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ResetHighlightedSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::RefetchDynamicModelNamesSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::HighlightFacingFurnitureSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::UpdateHeldItemPositionSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::UpdateHeldHandTruckPositionSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::UpdateVisualsForSettingsChangerSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessSquirterSystem>());
}
