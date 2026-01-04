#pragma once

namespace afterhours {
struct SystemManager;
}

namespace system_manager::ai {

// Runs early in the frame to reset AI scratch for freshly-entered states.
void register_ai_reset_system(afterhours::SystemManager& systems);

// Runs late in the frame to commit staged AI transitions.
void register_ai_commit_system(afterhours::SystemManager& systems);

}  // namespace system_manager::ai

