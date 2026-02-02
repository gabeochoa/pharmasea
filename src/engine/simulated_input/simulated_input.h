#pragma once

#include <filesystem>

#include "../../components/bypass_automation_state.h"
#include "../../entities/entity_helper.h"
#include "../../external_include.h"
#include "../../globals.h"
#include "../../network/network.h"
#include "../app.h"
#include "../files.h"
#include "../log.h"
#include "../settings.h"
#include "../statemanager.h"
#include "../svg_renderer.h"

namespace input_injector {
void schedule_mouse_click_at(const Rectangle& rect);
void inject_scheduled_click();
void release_scheduled_click();
void inject_key_press(int keycode);
void hold_key_for_duration(int keycode, float duration);
void set_key_down(int keycode);
void set_key_up(int keycode);
bool consume_synthetic_press(int keycode);
void update_key_hold(float dt);
bool is_key_synthetically_down(int keycode);
}  // namespace input_injector

namespace input_recorder {
void enable(const std::filesystem::path& path);
bool is_enabled();
void record_state_change(const char* label, int value);
void shutdown();
void update_poll();
}  // namespace input_recorder

namespace input_replay {
void start(const std::filesystem::path& path);
void stop();
bool is_active();
void update(float dt);
}  // namespace input_replay

namespace bypass_helper {
void inject_clicks_for_bypass(float dt);
}  // namespace bypass_helper

namespace simulated_input {
void init();
void update(float dt);
void stop();
void start_round_replay();
}  // namespace simulated_input
