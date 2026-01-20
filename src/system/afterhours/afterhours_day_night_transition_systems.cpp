// System includes - each system is in its own header file to improve build
// times
#include "day_night_transition/bypass_init_system.h"
#include "day_night_transition/clean_up_old_store_options_system.h"
#include "day_night_transition/delete_floating_items_when_leaving_in_round_system.h"
#include "day_night_transition/generate_store_options_system.h"
#include "day_night_transition/on_day_ended_system.h"
#include "day_night_transition/open_store_doors_system.h"
#include "day_night_transition/reset_has_day_night_changed.h"

namespace system_manager {}  // namespace system_manager

void SystemManager::register_day_night_transition_systems() {
    // Day/night transition systems - all check needs_to_process_change in
    // should_run(), reset clears the flag after processing
    {
#if ENABLE_DEV_FLAGS
        systems.register_update_system(
            std::make_unique<system_manager::BypassInitSystem>());
#endif
        // Day start systems
        {
            systems.register_update_system(
                std::make_unique<system_manager::GenerateStoreOptionsSystem>());
            systems.register_update_system(
                std::make_unique<system_manager::OpenStoreDoorsSystem>());
            systems.register_update_system(
                std::make_unique<
                    system_manager::
                        DeleteFloatingItemsWhenLeavingInRoundSystem>());
            systems.register_update_system(
                std::make_unique<system_manager::TellCustomersToLeaveSystem>());

            systems.register_update_system(
                std::make_unique<
                    system_manager::ResetToiletWhenLeavingInRoundSystem>());
            systems.register_update_system(
                std::make_unique<
                    system_manager::
                        ResetCustomerSpawnerWhenLeavingInRoundSystem>());
            systems.register_update_system(
                std::make_unique<
                    system_manager::UpdateNewMaxCustomersSystem>());
            systems.register_update_system(
                std::make_unique<system_manager::OnNightEndedTriggerSystem>());
            systems.register_update_system(
                std::make_unique<system_manager::OnDayStartedTriggerSystem>());
            systems.register_update_system(
                std::make_unique<
                    system_manager::OnRoundFinishedTriggerSystem>());
#if ENABLE_DEV_FLAGS
            systems.register_update_system(
                std::make_unique<system_manager::BypassRoundTrackerSystem>());
#endif
        }
        systems.register_update_system(
            std::make_unique<system_manager::CleanUpOldStoreOptionsSystem>());
        systems.register_update_system(
            std::make_unique<system_manager::OnDayEndedSystem>());
        systems.register_update_system(
            std::make_unique<
                system_manager::ResetRegisterQueueWhenLeavingInRoundSystem>());
        systems.register_update_system(
            std::make_unique<system_manager::CloseBuildingsWhenNightSystem>());
        systems.register_update_system(
            std::make_unique<system_manager::OnNightStartedSystem>());
        systems.register_update_system(
            std::make_unique<
                system_manager::ReleaseMopBuddyAtStartOfDaySystem>());
        systems.register_update_system(
            std::make_unique<
                system_manager::DeleteTrashWhenLeavingPlanningSystem>());
    }
    // This one needs to run after the transition systems to clear the flag
    systems.register_update_system(
        std::make_unique<system_manager::ResetHasDayNightChanged>());
}
