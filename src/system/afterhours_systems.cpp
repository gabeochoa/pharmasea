#include "afterhours_systems.h"

#include "system_manager.h"

void SystemManager::register_afterhours_systems() {
    register_sixtyfps_systems();
    register_gamelike_systems();
    register_modeltest_systems();
    register_inround_systems();
    register_planning_systems();
    register_render_systems();
}
