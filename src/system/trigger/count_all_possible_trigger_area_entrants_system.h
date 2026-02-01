#pragma once

#include "../../ah.h"
#include "../../components/is_trigger_area.h"
#include "../../entities/entity_query.h"
#include "../core/system_manager.h"

namespace system_manager {

struct CountAllPossibleTriggerAreaEntrantsSystem
    : public afterhours::System<IsTriggerArea> {
    int count = 0;

    virtual bool should_run(const float) override { return true; }

    virtual void once(float) override {
        count = static_cast<int>(EQ(SystemManager::get().oldAll)
                                     .whereType(EntityType::Player)
                                     .gen_count());
    }

    virtual void for_each_with(Entity&, IsTriggerArea& ita, float) override {
        ita.update_all_entrants(count);
    }
};

}  // namespace system_manager
