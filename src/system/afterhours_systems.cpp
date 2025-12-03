#include "afterhours_systems.h"

#include "system_manager.h"

// Individual system headers
#include "cart_management_system.h"
#include "delete_customers_when_leaving_inround_system.h"
#include "end_of_round_completion_validation_system.h"
#include "highlight_facing_furniture_system.h"
#include "pass_time_for_active_fishing_games_system.h"
#include "pass_time_for_transaction_animation_system.h"
#include "pop_out_when_colliding_system.h"
#include "process_ai_system.h"
#include "process_conveyer_items_system.h"
#include "process_day_start_system.h"
#include "process_floor_markers_system.h"
#include "process_grabber_filter_system.h"
#include "process_grabber_items_system.h"
#include "process_has_rope_system.h"
#include "process_is_container_and_should_backfill_item_system.h"
#include "process_is_container_and_should_update_item_system.h"
#include "process_is_indexed_container_holding_incorrect_item_system.h"
#include "process_night_start_system.h"
#include "process_nux_updates_system.h"
#include "process_pnumatic_pipe_movement_system.h"
#include "process_pnumatic_pipe_pairing_system.h"
#include "process_soda_fountain_system.h"
#include "process_spawner_system.h"
#include "process_squirter_system.h"
#include "process_trash_system.h"
#include "process_trigger_area_system.h"
#include "reduce_impatient_customers_system.h"
#include "refetch_dynamic_model_names_system.h"
#include "render_systems.h"
#include "reset_customers_that_need_resetting_system.h"
#include "reset_empty_work_furniture_system.h"
#include "reset_has_day_night_changed_system.h"
#include "reset_highlighted_system.h"
#include "run_timer_system.h"
#include "transform_snapper_system.h"
#include "update_character_model_from_index_system.h"
#include "update_held_furniture_position_system.h"
#include "update_held_hand_truck_position_system.h"
#include "update_held_item_position_system.h"
#include "update_visuals_for_settings_changer_system.h"

void SystemManager::register_sixtyfps_systems() {
    // This system should run in all states (lobby, game, model test, etc.)
    // because it handles trigger areas and other essential updates
    // Note: We run every frame for better responsiveness (especially for
    // trigger areas), which is acceptable since these operations are
    // lightweight. The original ran at 60fps (when timePassed >= 0.016f), but
    // running every frame ensures trigger areas and other interactions feel
    // more responsive.

    systems.register_update_system(
        std::make_unique<system_manager::ClearAllFloorMarkersSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::MarkItemInFloorAreaSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::UpdateDynamicTriggerAreaSettingsSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::CountAllPossibleTriggerAreaEntrantsSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::CountInBuildingTriggerAreaEntrantsSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::CountTriggerAreaEntrantsSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::UpdateTriggerAreaPercentSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::TriggerCbOnFullProgressSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessNuxUpdatesSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::UpdateCharacterModelFromIndexSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessSodaFountainSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessTrashSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::DeleteCustomersWhenLeavingInroundSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::TransformSnapperSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ResetHighlightedSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::RefetchDynamicModelNamesSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::HighlightFacingFurnitureSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::UpdateHeldItemPositionSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::UpdateHeldHandTruckPositionSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::UpdateVisualsForSettingsChangerSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessSquirterSystem>());
}

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

void SystemManager::register_day_night_transition_systems() {
    // Day/night transition systems - all check needs_to_process_change in
    // should_run(), reset clears the flag after processing
    {
        systems.register_update_system(
            std::make_unique<system_manager::ProcessDayStartSystem>());
        systems.register_update_system(
            std::make_unique<system_manager::ProcessNightStartSystem>());
    }
    // This one needs to run after the transition systems to clear the flag
    systems.register_update_system(
        std::make_unique<system_manager::ResetHasDayNightChanged>());
}

// Model test update system - processes entities during model test state
void SystemManager::register_modeltest_systems() {
    // should move all the container functions into its own
    // function?
    systems.register_update_system(
        std::make_unique<
            system_manager::ProcessIsContainerAndShouldUpdateItemSystem>());
    // This one should be after the other container ones
    // TODO before you migrate this, we need to look at the should_run logic
    // since the existing System<> uses is_nighttime
    systems.register_update_system(
        std::make_unique<
            system_manager::
                ProcessIsIndexedContainerHoldingIncorrectItemSystem>());

    systems.register_update_system(
        std::make_unique<
            system_manager::ProcessIsContainerAndShouldBackfillItemSystem>());
}

void SystemManager::register_inround_systems() {
    systems.register_update_system(
        std::make_unique<system_manager::InRoundUpdateSystem>());

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

void SystemManager::register_planning_systems() {
    systems.register_update_system(
        std::make_unique<system_manager::UpdateHeldFurniturePositionSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::CartManagementSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::PopOutWhenCollidingSystem>());
}

void SystemManager::register_render_systems() {
    system_manager::register_render_systems(systems);
}

void SystemManager::register_afterhours_systems() {
    register_sixtyfps_systems();
    register_gamelike_systems();
    register_modeltest_systems();
    register_inround_systems();
    register_planning_systems();
    register_render_systems();
}
