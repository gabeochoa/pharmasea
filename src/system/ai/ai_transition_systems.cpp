#include "ai_transition_systems.h"

// System includes - each system is in its own header file to improve build
// times
#include "transition/ai_commit_next_state_system.h"
#include "transition/ai_force_leave_commit_system.h"
#include "transition/ai_setup_state_components_system.h"
#include "transition/needs_bathroom_now_system.h"

namespace system_manager {

void register_ai_transition_systems(afterhours::SystemManager& systems) {
    // Set up state components for AI entities (adds missing, resets on
    // transition).
    systems.register_update_system(
        std::make_unique<system_manager::AISetupStateComponentsSystem>());
    // Bathroom override can preempt other transitions.
    systems.register_update_system(
        std::make_unique<system_manager::NeedsBathroomNowSystem>());
}

void register_ai_transition_commit_systems(afterhours::SystemManager& systems) {
    // Commit staged transitions after AI has had a chance to request them.
    systems.register_update_system(
        std::make_unique<system_manager::AICommitNextStateSystem>());
    // Force-leave override runs after normal commits.
    systems.register_update_system(
        std::make_unique<system_manager::AIForceLeaveCommitSystem>());
}

}  // namespace system_manager
