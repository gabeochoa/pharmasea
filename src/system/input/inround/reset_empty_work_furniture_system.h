#pragma once

#include "../../../ah.h"
#include "../../../components/can_hold_item.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../components/has_work.h"
#include "../../../entities/entity_helper.h"
#include "../../core/system_manager.h"

namespace system_manager {

namespace inround {

struct ResetEmptyWorkFurnitureSystem
    : public afterhours::System<HasWork, CanHoldItem> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_bar_open();
        } catch (...) {
            return false;
        }
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