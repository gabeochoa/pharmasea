#pragma once

#include "../../ah.h"

namespace system_manager {

// Registers AI setup and pre-processing systems (run before AI state systems):
// - AISetupStateComponentsSystem: ensures entities have required components
// - NeedsBathroomNowSystem: bathroom override
void register_ai_transition_systems(afterhours::SystemManager& systems);

// Registers AI commit systems (run after AI state systems):
// - AICommitNextStateSystem: commits staged transitions
// - AIForceLeaveCommitSystem: force-leave at end of round
void register_ai_transition_commit_systems(afterhours::SystemManager& systems);

}  // namespace system_manager
