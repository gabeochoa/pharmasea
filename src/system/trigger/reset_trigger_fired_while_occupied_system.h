#pragma once

#include "../../ah.h"
#include "../../components/is_trigger_area.h"
#include "../ai/ai_tags.h"

namespace system_manager {

struct ResetTriggerFiredWhileOccupiedSystem
    : public afterhours::System<
          IsTriggerArea,
          afterhours::tags::All<
              afterhours::tags::TriggerTag::GateTriggerWhileOccupied>> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsTriggerArea& ita,
                               float) override {
        if (ita.active_entrants() > 0) return;
        entity.disableTag(
            afterhours::tags::TriggerTag::TriggerFiredWhileOccupied);
    }
};

}  // namespace system_manager
