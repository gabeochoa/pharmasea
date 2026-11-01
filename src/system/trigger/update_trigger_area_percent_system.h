#pragma once

#include "../../ah.h"
#include "../../components/is_trigger_area.h"

namespace system_manager {

struct UpdateTriggerAreaPercentSystem
    : public afterhours::System<IsTriggerArea> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity&, IsTriggerArea& ita, float dt) override {
        ita.should_wave()  //
            ? ita.increase_progress(dt)
            : ita.decrease_progress(dt);

        if (!ita.should_progress()) ita.decrease_cooldown(dt);
    }
};

}  // namespace system_manager
