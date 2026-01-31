#include "afterhours_systems.h"

#include "afterhours/src/plugins/sound_system.h"
#include "../core/system_manager.h"

// Static singleton storage definitions for afterhours sound_system
std::shared_ptr<afterhours::sound_system::SoundLibrary>
    afterhours::sound_system::SoundLibrary_single;
std::shared_ptr<afterhours::sound_system::MusicLibrary>
    afterhours::sound_system::MusicLibrary_single;

void SystemManager::register_afterhours_systems() {
    register_sixtyfps_systems();
    register_gamelike_systems();
    register_modeltest_systems();
    register_inround_systems();
    register_planning_systems();
    register_render_systems();

    // Register sound system update (handles music stream updates)
    afterhours::sound_system::register_update_systems(systems);
}
