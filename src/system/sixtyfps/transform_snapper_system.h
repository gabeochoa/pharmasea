#pragma once

#include "../../ah.h"
#include "../../components/is_snappable.h"
#include "../../components/transform.h"

namespace system_manager {

// TODO split into two systems one for snappable and one for non-snappable
struct TransformSnapperSystem : public afterhours::System<Transform> {
    virtual void for_each_with(Entity& entity, Transform& transform,
                               float) override {
        transform.update(entity.has<IsSnappable>()  //
                             ? transform.snap_position()
                             : transform.raw());
    }
};

}  // namespace system_manager
