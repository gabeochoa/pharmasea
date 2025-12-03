#include "system_manager.h"

// Component includes needed for the moved struct definitions
#include "../ah.h"
#include "../components/can_be_held.h"
#include "../components/can_hold_furniture.h"
#include "../components/can_hold_handtruck.h"
#include "../components/has_day_night_timer.h"
#include "../components/is_bank.h"
#include "../components/is_floor_marker.h"
#include "../components/is_free_in_store.h"
#include "../components/is_solid.h"
#include "../components/is_store_spawned.h"
#include "../components/is_trigger_area.h"
#include "../components/transform.h"
#include "../engine/pathfinder.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "../entity_query.h"
#include "../entity_type.h"

namespace system_manager {

struct CartManagementSystem
    : public afterhours::System<
          IsFloorMarker, afterhours::tags::All<EntityType::FloorMarker>> {
    OptEntity sophie;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie->get<HasDayNightTimer>();
            return hastimer.is_daytime();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity&, IsFloorMarker& ifm, float) override {
        if (ifm.type != IsFloorMarker::Store_PurchaseArea) return;

        int amount_in_cart = 0;

        for (size_t i = 0; i < ifm.num_marked(); i++) {
            EntityID id = ifm.marked_ids()[i];
            OptEntity marked_entity = EntityHelper::getEntityForID(id);
            if (!marked_entity) continue;

            // Its free!
            if (marked_entity->has<IsFreeInStore>()) continue;
            // it was already purchased or is otherwise just randomly in the
            // store
            if (marked_entity->is_missing<IsStoreSpawned>()) continue;

            amount_in_cart +=
                std::max(0, get_price_for_entity_type(
                                get_entity_type(marked_entity.asE())));
        }

        if (sophie.has_value()) {
            IsBank& bank = sophie->get<IsBank>();
            bank.update_cart(amount_in_cart);
        }

        // Hack to force the validation function to run every frame
        OptEntity purchase_area = EntityHelper::getMatchingTriggerArea(
            IsTriggerArea::Type::Store_BackToPlanning);
        if (purchase_area.valid()) {
            (void) purchase_area->get<IsTriggerArea>().should_progress();
        }
    }
};

struct PopOutWhenCollidingSystem
    : public afterhours::System<Transform, CanHoldHandTruck, CanHoldFurniture> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            return hastimer.is_daytime();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity& entity, Transform& transform,
                               CanHoldHandTruck& chht, CanHoldFurniture& chf,
                               float) override {
        const auto no_clip_on =
            GLOBALS.get_or_default<bool>("no_clip_enabled", false);
        if (no_clip_on) return;

        // Only popping out players right now
        if (!check_type(entity, EntityType::Player)) return;

        OptEntity match =  //
            EntityQuery()
                .whereNotID(entity.id)  // not us
                .whereHasComponent<IsSolid>()
                .whereInRange(transform.as2(), 0.7f)
                .whereLambdaExistsAndTrue([](const Entity& other) {
                    // this filter isnt for you
                    if (other.is_missing<CanBeHeld_HT>()) return true;
                    // ignore if held
                    return !other.get<CanBeHeld_HT>().is_held();
                })
                .include_store_entities()
                .gen_first();

        if (!match) {
            return;
        }

        if (chht.is_holding()) {
            OptEntity hand_truck =
                EntityHelper::getEntityForID(chht.hand_truck_id());
            if (match->id == chht.hand_truck_id()) {
                return;
            }
            if (chht.is_holding() &&
                match->id ==
                    hand_truck->get<CanHoldFurniture>().furniture_id()) {
                return;
            }
        }

        if (chf.is_holding_furniture()) return;
        if (chf.furniture_id() == match->id) return;

        vec2 new_position = transform.as2();

        int i = static_cast<int>(new_position.x);
        int j = static_cast<int>(new_position.y);
        for (int a = 0; a < 8; a++) {
            auto position = (vec2{(float) i + (bfs::neigh_x[a]),
                                  (float) j + (bfs::neigh_y[a])});
            if (EntityHelper::isWalkable(position)) {
                new_position = position;
                break;
            }
        }

        transform.update(vec::to3(new_position));
    }
};

struct UpdateHeldFurniturePositionSystem
    : public afterhours::System<CanHoldFurniture, Transform> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;

        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
        return hastimer.is_daytime();
    }

    virtual void for_each_with([[maybe_unused]] Entity& entity,
                               CanHoldFurniture& can_hold_furniture,
                               Transform& transform, float) override {
        if (can_hold_furniture.empty()) return;

        auto new_pos = transform.pos();
        if (transform.face_direction() &
            Transform::FrontFaceDirection::FORWARD) {
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

        OptEntity furniture =
            EntityHelper::getEntityForID(can_hold_furniture.furniture_id());
        furniture->get<Transform>().update(new_pos);
    }
};

}  // namespace system_manager

void SystemManager::register_planning_systems() {
    systems.register_update_system(
        std::make_unique<system_manager::UpdateHeldFurniturePositionSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::CartManagementSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::PopOutWhenCollidingSystem>());
}
