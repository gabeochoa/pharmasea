// System includes - each system is in its own header file to improve build
// times
#include "gamelike/end_of_round_completion_validation_system.h"
#include "gamelike/pass_time_for_transaction_animation_system.h"
#include "gamelike/process_is_container_and_should_backfill_item_system.h"
#include "gamelike/process_pnumatic_pipe_pairing_system.h"
#include "gamelike/run_timer_system.h"

#include "../ai/ai_system.h"
#include "../ai/ai_transition_systems.h"
#include "../ai/process_ai_system.h"

namespace system_manager {

}  // namespace system_manager

void SystemManager::register_gamelike_systems() {
    systems.register_update_system(
        std::make_unique<system_manager::RunTimerSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessPnumaticPipePairingSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::ProcessIsContainerAndShouldBackfillItemSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::PassTimeForTransactionAnimationSystem>());

    // AI systems: setup -> state processing -> commit
    system_manager::register_ai_transition_systems(systems);
    system_manager::register_ai_systems(systems);
    system_manager::register_ai_transition_commit_systems(systems);

    systems.register_update_system(
        std::make_unique<
            system_manager::EndOfRoundCompletionValidationSystem>());

    register_day_night_transition_systems();
}