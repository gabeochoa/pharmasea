#pragma once

#include "../../../ah.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../building_locations.h"
#include "../../../components/is_solid.h"
#include "../../../engine/statemanager.h"
#include "../../../entity_helper.h"
#include "../../core/system_manager.h"

namespace system_manager {

struct OpenStoreDoorsSystem
    : public afterhours::System<IsSolid,
                                afterhours::tags::All<EntityType::Door>> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
            return timer.needs_to_process_change && timer.is_bar_closed();
        } catch (...) {
            return false;
        }
    }
    virtual void for_each_with(Entity& entity, IsSolid&, float) override {
        if (!CheckCollisionBoxes(entity.get<Transform>().bounds(),
                                 STORE_BUILDING.bounds))
            return;
        entity.removeComponentIfExists<IsSolid>();
    }
};

}  // namespace system_manager