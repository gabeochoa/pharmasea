#pragma once

#include "../../../ah.h"
#include "../../../components/can_hold_item.h"
#include "../../../components/has_work.h"
#include "../../core/system_manager.h"
#include "../../system_utilities.h"

namespace system_manager {

namespace inround {

struct ResetEmptyWorkFurnitureSystem
    : public afterhours::System<HasWork, CanHoldItem> {
    virtual bool should_run(const float) override {
        return system_utils::should_run_inround_system();
    }

    void for_each_with(Entity&, HasWork& hasWork, CanHoldItem& chi,
                       float) override {
        if (!hasWork.should_reset_on_empty()) return;
        if (chi.empty()) {
            hasWork.reset_pct();
            return;
        }
    }
};

}  // namespace inround

}  // namespace system_manager