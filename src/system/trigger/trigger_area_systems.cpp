// System includes - each system is in its own header file to improve build
// times
#include "count_all_possible_trigger_area_entrants_system.h"
#include "count_in_building_trigger_area_entrants_system.h"
#include "count_trigger_area_entrants_system.h"
#include "mark_trigger_area_full_needs_processing_system.h"
#include "reset_trigger_fired_while_occupied_system.h"
#include "trigger_cb_on_full_progress_system.h"
#include "update_dynamic_trigger_area_settings_system.h"
#include "update_trigger_area_percent_system.h"

namespace system_manager {

// Global state for load/save delete mode
bool g_load_save_delete_mode = false;

// Forward declarations for functions implemented elsewhere
void generate_machines_for_new_upgrades();
void spawn_machines_for_newly_unlocked_drink_DONOTCALL(IsProgressionManager&,
                                                       Drink);
namespace progression {
void update_upgrade_variables();
}  // namespace progression
namespace store {
void cleanup_old_store_options();
void generate_store_options();
void move_purchased_furniture();
}  // namespace store

void register_trigger_area_systems(afterhours::SystemManager& systems) {
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
        std::make_unique<
            system_manager::ResetTriggerFiredWhileOccupiedSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::MarkTriggerAreaFullNeedsProcessingSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::TriggerCbOnFullProgressSystem>());
}

}  // namespace system_manager
