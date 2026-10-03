// System includes - each system is in its own header file to improve build
// times
#include "process_ai_system.h"

#include "ai_at_register_wait_for_drink_system.h"
#include "ai_bathroom_system.h"
#include "ai_clean_vomit_system.h"
#include "ai_drinking_system.h"
#include "ai_leave_system.h"
#include "ai_pay_system.h"
#include "ai_play_jukebox_system.h"
#include "ai_queue_for_register_system.h"
#include "ai_shared_utilities.h"
#include "ai_wander_system.h"

namespace system_manager {

void register_ai_systems(afterhours::SystemManager& systems) {
    systems.register_update_system(std::make_unique<AIWanderSystem>());
    systems.register_update_system(
        std::make_unique<AIQueueForRegisterSystem>());
    systems.register_update_system(
        std::make_unique<AIAtRegisterWaitForDrinkSystem>());
    systems.register_update_system(std::make_unique<AIDrinkingSystem>());
    systems.register_update_system(std::make_unique<AIPaySystem>());
    systems.register_update_system(std::make_unique<AIPlayJukeboxSystem>());
    systems.register_update_system(std::make_unique<AIBathroomSystem>());
    systems.register_update_system(std::make_unique<AICleanVomitSystem>());
    systems.register_update_system(std::make_unique<AILeaveSystem>());
}

}  // namespace system_manager
