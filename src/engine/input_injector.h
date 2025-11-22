#pragma once

#include "app.h"
#include "event.h"
#include "keymap.h"
#include "ui/focus.h"

// Helper to inject synthetic input events for bypass functionality
namespace input_injector {

// Schedule a mouse click to be injected (called before draw)
void schedule_mouse_click_at(const Rectangle& rect);

// Inject the scheduled click (must be called after ui::begin())
void inject_scheduled_click();

// Release the scheduled click (must be called after buttons are checked, in
// ui::end())
void release_scheduled_click();

// Legacy function for backwards compatibility
inline void inject_mouse_click_at(const Rectangle& rect) {
    schedule_mouse_click_at(rect);
}

// Inject a keyboard key press
inline void inject_key_press(int keycode) {
    KeyPressedEvent event(keycode, 0);
    App::get().processEvent(event);
}

// Start holding a key for a specific duration (call update_key_hold each frame)
void hold_key_for_duration(int keycode, float duration);

// Update key hold state (call this each frame)
void update_key_hold(float dt);

// Check if a key is synthetically held down (for movement system)
bool is_key_synthetically_down(int keycode);

}  // namespace input_injector
