// System includes - each system is in its own header file to improve build
// times
#include "../core/system_manager.h"
#include "../trigger/trigger_area_systems.h"
#include "clear_floor_markers_system.h"
#include "delete_customers_system.h"
#include "highlight_facing_furniture_system.h"
#include "mark_floor_area_system.h"
#include "process_nux_updates_system.h"
#include "process_soda_fountain_system.h"
#include "process_squirter_system.h"
#include "process_trash_system.h"
#include "refetch_dynamic_model_names_system.h"
#include "reset_highlighted_system.h"
#include "show_minimap_when_highlighted_system.h"
#include "transform_snapper_system.h"
#include "update_character_model_system.h"
#include "update_held_position_system.h"
#include "update_settings_changer_system.h"

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
    system_manager::register_trigger_area_systems(systems);
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
        std::make_unique<system_manager::ShowMinimapWhenHighlightedSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::UpdateHeldPositionSystem<CanHoldItem>>());
    systems.register_update_system(
        std::make_unique<system_manager::UpdateHeldPositionSystem<CanHoldHandTruck>>());
    systems.register_update_system(
        std::make_unique<
            system_manager::UpdateVisualsForSettingsChangerSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessSquirterSystem>());
}
