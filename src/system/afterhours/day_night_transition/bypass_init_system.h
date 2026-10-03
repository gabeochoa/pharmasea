#pragma once

#include "../../../ah.h"
#include "../../../components/bypass_automation_state.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../entities/entity_helper.h"
#include "../../core/system_manager.h"

namespace system_manager {

struct BypassInitSystem : public afterhours::System<BypassAutomationState> {
    virtual bool should_run(const float) override {
        if (!BYPASS_MENU && BYPASS_ROUNDS <= 0) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            sophie.addComponentIfMissing<BypassAutomationState>();
            return true;
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity&, BypassAutomationState& state,
                               float) override {
        if (state.initialized) return;
        int rounds_to_run = BYPASS_ROUNDS;
        if (rounds_to_run < 1 && BYPASS_MENU) {
            rounds_to_run = 1;
        }
        if (rounds_to_run <= 0) return;
        state.configure(rounds_to_run, EXIT_ON_BYPASS_COMPLETE, RECORD_INPUTS);
        BYPASS_MENU = state.bypass_enabled;
    }
};

}  // namespace system_manager