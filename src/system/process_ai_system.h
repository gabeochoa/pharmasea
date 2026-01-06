#pragma once

#include "../components/can_pathfind.h"
#include "../components/is_ai_controlled.h"
#include "ai_tags.h"

namespace system_manager {

// TODO should we have a separate system for each job type?
//
// Note: afterhours tag filtering currently only applies on Apple platforms
// (see vendor/afterhours/src/core/system.h). We still guard at runtime on other
// platforms.
struct ProcessAiSystem
    : public afterhours::System<
          IsAIControlled, CanPathfind,
          afterhours::tags::None<afterhours::tags::AITag::AITransitionPending,
                                 afterhours::tags::AITag::AINeedsResetting>> {
    virtual bool should_run(const float) override;

    virtual void for_each_with(Entity& entity, IsAIControlled& ctrl,
                               [[maybe_unused]] CanPathfind&, float dt) override;
};

}  // namespace system_manager

