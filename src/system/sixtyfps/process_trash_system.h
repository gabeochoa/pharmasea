#pragma once

#include "../../ah.h"
#include "../../components/can_hold_item.h"
#include "../../entity_helper.h"
#include "../../entity_id.h"

namespace system_manager {

struct ProcessTrashSystem : public afterhours::System<CanHoldItem> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, CanHoldItem& trashCHI,
                               float) override {
        if (!check_type(entity, EntityType::Trash)) return;

        // If we arent holding anything, nothing to delete
        if (trashCHI.empty()) return;

        OptEntity item_opt = trashCHI.item();
        if (!item_opt) {
            trashCHI.update(nullptr, entity_id::INVALID);
            return;
        }
        item_opt.asE().cleanup = true;
        trashCHI.update(nullptr, entity_id::INVALID);
    }
};

}  // namespace system_manager
