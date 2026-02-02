#include "../core/system_manager.h"

// Individual system headers for render systems
#include "../rendering/rendering_system.h"

void SystemManager::register_render_systems() {
    // TODO inline
    system_manager::register_render_systems(systems);
}
