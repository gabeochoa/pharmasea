#include "system_manager.h"

// Individual system headers for inround systems
#include "pass_time_for_active_fishing_games_system.h"
#include "process_conveyer_items_system.h"
#include "process_grabber_filter_system.h"
#include "process_grabber_items_system.h"
#include "process_has_rope_system.h"
#include "process_is_container_and_should_update_item_system.h"
#include "process_is_indexed_container_holding_incorrect_item_system.h"
#include "process_pnumatic_pipe_movement_system.h"
#include "process_spawner_system.h"
#include "reduce_impatient_customers_system.h"
#include "reset_customers_that_need_resetting_system.h"
#include "reset_empty_work_furniture_system.h"

void SystemManager::register_inround_systems() {
    systems.register_update_system(
        std::make_unique<
            system_manager::ResetCustomersThatNeedResettingSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessGrabberItemsSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessConveyerItemsSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessGrabberFilterSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessHasRopeSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessPnumaticPipeMovementSystem>());
    // should move all the container functions into its own
    // function?
    systems.register_update_system(
        std::make_unique<
            system_manager::ProcessIsContainerAndShouldUpdateItemSystem>());
    // This one should be after the other container ones
    systems.register_update_system(
        std::make_unique<
            system_manager::
                ProcessIsIndexedContainerHoldingIncorrectItemSystem>());

    systems.register_update_system(
        std::make_unique<system_manager::ProcessSpawnerSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ResetEmptyWorkFurnitureSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ReduceImpatientCustomersSystem>());

    systems.register_update_system(
        std::make_unique<
            system_manager::PassTimeForActiveFishingGamesSystem>());
}
