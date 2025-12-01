#include "afterhours_systems.h"

#include "system_manager.h"

// Individual system headers
#include "delete_customers_when_leaving_inround_system.h"
#include "highlight_facing_furniture_system.h"
#include "pass_time_for_transaction_animation_system.h"
#include "process_ai_system.h"
#include "process_floor_markers_system.h"
#include "process_is_container_and_should_backfill_item_system.h"
#include "process_nux_updates_system.h"
#include "process_pnumatic_pipe_pairing_system.h"
#include "process_soda_fountain_system.h"
#include "process_squirter_system.h"
#include "process_trash_system.h"
#include "process_trigger_area_system.h"
#include "refetch_dynamic_model_names_system.h"
#include "reset_highlighted_system.h"
#include "run_timer_system.h"
#include "transform_snapper_system.h"
#include "update_character_model_from_index_system.h"
#include "update_held_furniture_position_system.h"
#include "update_held_hand_truck_position_system.h"
#include "update_held_item_position_system.h"
#include "update_sophie_system.h"
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
    systems.register_update_system(
        std::make_unique<system_manager::SixtyFpsUpdateSystem>());
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
        std::make_unique<system_manager::UpdateSophieSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::GameLikeUpdateSystem>());
}

void SystemManager::register_modeltest_systems() {
    systems.register_update_system(
        std::make_unique<system_manager::ModelTestUpdateSystem>());
}

void SystemManager::register_inround_systems() {
    systems.register_update_system(
        std::make_unique<system_manager::InRoundUpdateSystem>());
}

void SystemManager::register_planning_systems() {
    systems.register_update_system(
        std::make_unique<system_manager::UpdateHeldFurniturePositionSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::PlanningUpdateSystem>());
}

void SystemManager::register_render_systems() {
    systems.register_render_system(
        std::make_unique<system_manager::RenderEntitiesSystem>());
}

void SystemManager::register_afterhours_systems() {
    register_sixtyfps_systems();
    register_gamelike_systems();
    register_modeltest_systems();
    register_inround_systems();
    register_planning_systems();
    register_render_systems();
}
