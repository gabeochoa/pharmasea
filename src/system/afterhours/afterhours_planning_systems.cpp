#include "../input/input_process_manager.h"
#include "../core/system_manager.h"

void SystemManager::register_planning_systems() {
    system_manager::input_process_manager::planning::register_input_systems(
        systems);
}
