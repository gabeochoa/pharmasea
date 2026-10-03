#pragma once

// Thin wrapper around afterhours' input_injector.
// Keeps the pharmasea `input_injector::` API so callers don't change,
// but all state lives in afterhours::testing::input_injector.

#include "../../../vendor/afterhours/src/plugins/e2e_testing/input_injector.h"

namespace ah_injector = afterhours::testing::input_injector;

namespace input_injector {

inline void set_key_down(int keycode) { ah_injector::set_key_down(keycode); }
inline void set_key_up(int keycode) { ah_injector::set_key_up(keycode); }
inline bool is_key_synthetically_down(int keycode) {
    return ah_injector::is_key_down(keycode);
}
inline bool consume_synthetic_press(int keycode) {
    return ah_injector::consume_press(keycode);
}
inline void hold_key_for_duration(int keycode, float duration) {
    ah_injector::hold_key_for_duration(keycode, duration);
}
inline void update_key_hold(float dt) { ah_injector::update_key_hold(dt); }

inline void schedule_mouse_click_at(const Rectangle& rect) {
    ah_injector::schedule_click_at(rect.x, rect.y, rect.width, rect.height);
}

// inject/release need the ui::focus bridge, but focus.h has a circular
// dependency through this header. The bridge is done in inject_scheduled_click
// and release_scheduled_click which are called from ui.h/ui.cpp where
// focus.h is already available. We just delegate to afterhours here.
inline void inject_scheduled_click() { ah_injector::inject_scheduled_click(); }

inline void release_scheduled_click() {
    ah_injector::release_scheduled_click();
}

inline void reset_frame() { ah_injector::reset_frame(); }

}  // namespace input_injector
