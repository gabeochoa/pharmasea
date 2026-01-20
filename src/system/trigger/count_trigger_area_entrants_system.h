#pragma once

#include "../../ah.h"
#include "../../components/is_trigger_area.h"
#include "../../components/transform.h"
#include "../../entity_query.h"
#include "../core/system_manager.h"

namespace system_manager {

struct CountTriggerAreaEntrantsSystem
    : public afterhours::System<IsTriggerArea> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsTriggerArea& ita,
                               float) override {
        size_t count =
            EQ(SystemManager::get().oldAll)
                .whereType(EntityType::Player)
                .whereCollides(
                    entity.get<Transform>().expanded_bounds({0, TILESIZE, 0}))
                .gen_count();

        ita.update_entrants(static_cast<int>(count));
    }
};

}  // namespace system_manager
