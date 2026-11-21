#pragma once

#include "../../ah.h"
#include "../../components/is_trigger_area.h"
#include "../ai/ai_tags.h"

namespace system_manager {

struct MarkTriggerAreaFullNeedsProcessingSystem
    : public afterhours::System<IsTriggerArea> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsTriggerArea& ita,
                               float) override {
        if (ita.progress() < 1.f) return;
        entity.enableTag(
            afterhours::tags::TriggerTag::TriggerAreaFullNeedsProcessing);
    }
};

}  // namespace system_manager
