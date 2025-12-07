#include "../ah.h"
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
#include "../components/is_drink.h"
#include "../components/is_floor_marker.h"
#include "../components/is_nux_manager.h"
#include "../components/is_pnumatic_pipe.h"
#include "../components/is_round_settings_manager.h"
#include "../components/is_snappable.h"
#include "../components/is_solid.h"
#include "../components/is_squirter.h"
#include "../components/is_trigger_area.h"
#include "../components/model_renderer.h"
#include "../components/simple_colored_box_renderer.h"
#include "../components/transform.h"
#include "../components/uses_character_model.h"
#include "../dataclass/ingredient.h"
#include "../engine/bitset_utils.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "../entity_makers.h"
#include "../entity_query.h"
#include "../external_include.h"
#include "../vec_util.h"
#include "process_trigger_area_system.h"
#include "system_manager.h"

namespace system_manager {

bool _create_nuxes(Entity& entity);
void process_nux_updates(Entity& entity, float dt);

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
