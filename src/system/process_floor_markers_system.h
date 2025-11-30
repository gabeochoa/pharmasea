#pragma once

#include "../ah.h"
#include "../components/is_floor_marker.h"
#include "../components/is_solid.h"
#include "../components/transform.h"
#include "../entity_helper.h"
#include "../entity_query.h"
#include "system_manager.h"

namespace system_manager {

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

}  // namespace system_manager
