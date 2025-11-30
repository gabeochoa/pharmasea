#pragma once

#include "../ah.h"
#include "../components/can_hold_item.h"
#include "../entity_helper.h"
#include "system_manager.h"

namespace system_manager {

struct ProcessTrashSystem : public afterhours::System<CanHoldItem> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, CanHoldItem& trashCHI,
                               float) override {
        if (!check_type(entity, EntityType::Trash)) return;

        // If we arent holding anything, nothing to delete
        if (trashCHI.empty()) return;

        trashCHI.item().cleanup = true;
        trashCHI.update(nullptr, -1);
    }
};

}  // namespace system_manager
