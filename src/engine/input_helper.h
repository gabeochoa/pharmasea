// src/engine/input_helper.h
#pragma once

#include <vector>

#include "keymap.h"  // For InputName enum (keep this)

namespace input_helper {

// Initialize the input system (call during app startup)
void init();

// Sync the active layer to match the current menu state (call on state change)
void sync_layer();

// Poll raw input for the current frame (call at start of each frame)
void poll(float dt);

// Query if an input action is currently held (returns 0.0-1.0 for analog)
// Uses current menu state to determine layer
[[nodiscard]] float is_down(InputName name);

// Query if an input action is currently held for a specific layer
// Use this when you know which layer you want regardless of current state
[[nodiscard]] float is_down_for_layer(menu::State layer, InputName name);

// Query if an input action was just pressed this frame
// Uses current menu state to determine layer
[[nodiscard]] bool was_pressed(InputName name);

// Query if an input action was just pressed for a specific layer
[[nodiscard]] bool was_pressed_for_layer(menu::State layer, InputName name);

// Get all inputs that were pressed this frame (for UI iteration)
struct InputEvent {
    InputName name;
    float amount;
    int gamepad_id;
};
[[nodiscard]] std::vector<InputEvent> get_pressed_this_frame();

}  // namespace input_helper
