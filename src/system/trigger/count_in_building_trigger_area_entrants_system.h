#pragma once

#include <unordered_map>

#include "../../ah.h"
#include "../../building_locations.h"
#include "../../components/is_trigger_area.h"
#include "../../entity_query.h"
#include "../core/system_manager.h"

namespace system_manager {

struct CountInBuildingTriggerAreaEntrantsSystem
    : public afterhours::System<IsTriggerArea> {
    std::unordered_map<BuildingType, int> counts;

    virtual bool should_run(const float) override { return true; }

    virtual void once(float) override {
        for (BuildingType type : magic_enum::enum_values<BuildingType>()) {
            const Building& b = get_building(type);
            counts[type] = static_cast<int>(EQ(SystemManager::get().oldAll)
                                                .whereType(EntityType::Player)
                                                .whereInside(b.min(), b.max())
                                                .gen_count());
        }
    }

    virtual void for_each_with(Entity&, IsTriggerArea& ita, float) override {
        if (!ita.building) return;
        ita.update_entrants_in_building(counts[ita.building->id]);
    }
};

}  // namespace system_manager
