#pragma once

#include "../ah.h"
#include "../components/can_hold_item.h"
#include "../components/is_drink.h"
#include "../entity_helper.h"
#include "system_manager.h"

namespace system_manager {

// Forward declaration for helper function in system_manager.cpp
void process_soda_fountain(Entity& entity, float dt);

struct ProcessSodaFountainSystem : public afterhours::System<CanHoldItem> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, CanHoldItem& sfCHI,
                               float) override {
        if (!check_type(entity, EntityType::SodaFountain)) return;

        // If we arent holding anything, nothing to squirt into
        if (sfCHI.empty()) return;

        if (sfCHI.item().is_missing<IsDrink>()) return;

        // Forward to full implementation
        process_soda_fountain(entity, 0.0f);
    }
};

}  // namespace system_manager
