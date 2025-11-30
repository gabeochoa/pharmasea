#pragma once

#include "../ah.h"
#include "../components/can_be_highlighted.h"
#include "../components/can_highlight_others.h"
#include "../components/transform.h"
#include "../entity_helper.h"
#include "../entity_query.h"

namespace system_manager {

struct HighlightFacingFurnitureSystem
    : public afterhours::System<CanHighlightOthers, Transform> {
    virtual void for_each_with(Entity& entity, CanHighlightOthers& cho,
                               Transform& transform, float) override {
        OptEntity match = EntityQuery()
                              .whereHasComponent<CanBeHighlighted>()
                              .whereInRange(transform.as2(), cho.reach())
                              .include_store_entities()
                              .orderByDist(transform.as2())
                              .gen_first();
        if (!match) return;

        match->get<CanBeHighlighted>().update(entity, true);
    }
};

}  // namespace system_manager
