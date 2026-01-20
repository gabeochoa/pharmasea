#pragma once

#include "../../ah.h"
#include "../../components/is_floor_marker.h"
#include "../../entity_helper.h"

namespace system_manager {

struct ClearAllFloorMarkersSystem : public afterhours::System<IsFloorMarker> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsFloorMarker& ifm,
                               float) override {
        if (!check_type(entity, EntityType::FloorMarker)) return;
        ifm.clear();
    }
};

}  // namespace system_manager
