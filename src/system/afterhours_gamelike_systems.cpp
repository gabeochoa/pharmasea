#include "system_manager.h"

// Individual system headers for gamelike systems
#include "end_of_round_completion_validation_system.h"
#include "pass_time_for_transaction_animation_system.h"
#include "process_ai_system.h"
#include "process_is_container_and_should_backfill_item_system.h"
#include "process_pnumatic_pipe_pairing_system.h"
#include "run_timer_system.h"

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
    systems.register_update_system(
        std::make_unique<system_manager::ProcessAiSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::EndOfRoundCompletionValidationSystem>());

    register_day_night_transition_systems();
}
