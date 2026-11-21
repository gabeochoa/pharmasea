// src/engine/input_helper.h
#pragma once

#include <vector>

#include "keymap.h"  // For InputName enum (keep this)

namespace input_helper {

// Initialize the input system (call during app startup)
void init();

// Poll raw input for the current frame (call at start of each frame)
void poll(float dt);

// Query if an input action is currently held (returns 0.0-1.0 for analog)
[[nodiscard]] float is_down(InputName name);

// Query if an input action was just pressed this frame
[[nodiscard]] bool was_pressed(InputName name);

// Consume a pressed input so subsequent was_pressed() calls return false
// Use this to prevent multiple layers from responding to the same input
void consume_pressed(InputName name);

// Get all inputs that were pressed this frame (for UI iteration)
struct InputEvent {
    InputName name;
    float amount;
    int gamepad_id;
};
[[nodiscard]] std::vector<InputEvent> get_pressed_this_frame();

}  // namespace input_helper
