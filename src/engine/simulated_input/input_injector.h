#pragma once

#include "../app.h"
#include "../keymap.h"
#include "../ui/focus.h"

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
