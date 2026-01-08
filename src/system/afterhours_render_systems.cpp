#include "system_manager.h"

// Individual system headers for render systems
#include "rendering_system.h"

void SystemManager::register_render_systems() {
    system_manager::register_render_systems(systems);
}
