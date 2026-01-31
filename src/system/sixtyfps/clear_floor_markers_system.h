#pragma once

#include "../../ah.h"
#include "../../components/is_floor_marker.h"
#include "../../entity_helper.h"

namespace system_manager {

struct ClearAllFloorMarkersSystem
    : public afterhours::System<
          IsFloorMarker, afterhours::tags::All<EntityType::FloorMarker>> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with([[maybe_unused]] Entity&, IsFloorMarker& ifm,
                               float) override {
        ifm.clear();
    }
};

}  // namespace system_manager
