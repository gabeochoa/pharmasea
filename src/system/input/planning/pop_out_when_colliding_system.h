#pragma once

#include <algorithm>

#include "../../../ah.h"
#include "../../../components/can_be_held.h"
#include "../../../components/can_hold_furniture.h"
#include "../../../components/can_hold_handtruck.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../components/transform.h"
#include "../../../entity_helper.h"
#include "../../../entity_query.h"
#include "../../../entity_type.h"
#include "../../core/system_manager.h"

namespace system_manager {

namespace planning {

struct PopOutWhenCollidingSystem
    : public afterhours::System<Transform, CanHoldHandTruck, CanHoldFurniture> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            return hastimer.is_bar_closed();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity& entity, Transform& transform,
                               CanHoldHandTruck& chht, CanHoldFurniture& chf,
                               float) override {
        const auto no_clip_on = globals::no_clip_enabled();
        // TODO: move this check into should_run
        if (no_clip_on) return;

        // TODO: move this check into sysmem tags
        if (!check_type(entity, EntityType::Player)) return;

        OptEntity match =  //
            EntityQuery()
                .whereNotID(entity.id)
                .whereHasComponent<IsSolid>()
                .whereInRange(transform.as2(), 0.7f)
                .whereLambdaExistsAndTrue([](const Entity& other) {
                    if (other.is_missing<CanBeHeld_HT>()) return true;
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

}  // namespace planning

}  // namespace system_manager