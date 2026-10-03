#pragma once

#include "../../ah.h"
#include "../../components/transform.h"
#include "held_position_traits.h"

namespace system_manager {

template <typename HolderComponent>
struct UpdateHeldPositionSystem
    : public afterhours::System<HolderComponent, Transform> {

    using Entity = afterhours::Entity;
    using Traits = HeldPositionTraits<HolderComponent>;

    virtual void for_each_with(Entity& entity, HolderComponent& holder,
                               Transform&, float) override {
        if (holder.empty()) return;

        vec3 new_pos = Traits::get_position(entity);

        OptEntity held_opt = Traits::get_held_entity(holder);
        if (!held_opt) {
            Traits::clear_held(holder, entity.id);
            return;
        }
        held_opt.asE().get<Transform>().update(new_pos);
    }
};

}  // namespace system_manager
