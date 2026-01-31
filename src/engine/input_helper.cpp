// src/engine/input_helper.cpp
//
// Provides input abstraction using afterhours layered input system.
// Polls all layers (Game and UI) so inputs work regardless of menu state.
//
// NOTE: We implement our own input checking instead of using afterhours'
// check_single_action_down/pressed functions because we need to use
// pharmasea's ext::is_key_down/is_key_pressed wrappers for replay
// system compatibility (synthetic key injection).
//

#include "input_helper.h"

#include <algorithm>

#include "afterhours/src/plugins/input_system.h"
#include "graphics.h"  // For ext::is_key_down, ext::is_key_pressed
#include "keymap.h"
#include "magic_enum/magic_enum.hpp"

namespace input_helper {

namespace {

// Static storage for layered input
using LayerMapping = std::map<int, afterhours::input::ValidInputs>;
using FullMapping = std::map<menu::State, LayerMapping>;

FullMapping g_mapping;

// Input state for current frame
afterhours::input::InputCollector g_collector;
int g_max_gamepad = 0;
bool g_initialized = false;

// Convert pharmasea GamepadAxisWithDir to afterhours format
afterhours::input::GamepadAxisWithDir convert_axis(
    const GamepadAxisWithDir& src) {
    return afterhours::input::GamepadAxisWithDir{
        .axis = src.axis, .dir = static_cast<int>(src.dir)};
}

// Local input checking that uses pharmasea's ext:: wrappers
// This ensures synthetic keys from replay work correctly
namespace local_input {

constexpr float DEADZONE = 0.25f;

float check_key_down(int keycode) {
    // Use ext::is_key_down which checks both real and synthetic keys
    return ext::is_key_down(keycode) ? 1.f : 0.f;
}

float check_key_pressed(int keycode) {
    // Use ext::is_key_pressed which checks both real and synthetic keys
    return ext::is_key_pressed(keycode) ? 1.f : 0.f;
}

float check_axis(int gamepad_id,
                 const afterhours::input::GamepadAxisWithDir& axis_with_dir) {
    const float mvt =
        raylib::GetGamepadAxisMovement(gamepad_id, axis_with_dir.axis);
    if (axis_with_dir.dir > 0 && mvt > DEADZONE) {
        return mvt;
    }
    if (axis_with_dir.dir < 0 && mvt < -DEADZONE) {
        return -mvt;
    }
    return 0.f;
}

float check_button_down(int gamepad_id, raylib::GamepadButton button) {
    return raylib::IsGamepadButtonDown(gamepad_id, button) ? 1.f : 0.f;
}

float check_button_pressed(int gamepad_id, raylib::GamepadButton button) {
    return raylib::IsGamepadButtonPressed(gamepad_id, button) ? 1.f : 0.f;
}

// Returns {medium, value} for "down" state
std::pair<afterhours::input::DeviceMedium, float> check_action_down(
    int gamepad_id, const afterhours::input::ValidInputs& valid_inputs) {
    using Medium = afterhours::input::DeviceMedium;
    Medium medium = Medium::None;
    float value = 0.f;

    for (const auto& input : valid_inputs) {
        Medium temp_medium = Medium::None;
        float temp = 0.f;

        if (input.index() == 0) {
            temp_medium = Medium::Keyboard;
            temp = check_key_down(std::get<0>(input));
        } else if (input.index() == 1) {
            temp_medium = Medium::GamepadAxis;
            temp = check_axis(gamepad_id, std::get<1>(input));
        } else if (input.index() == 2) {
            temp_medium = Medium::GamepadButton;
            temp = check_button_down(gamepad_id, std::get<2>(input));
        }

        if (temp > value) {
            value = temp;
            medium = temp_medium;
        }
    }
    return {medium, value};
}

// Returns {medium, value} for "pressed" state (just this frame)
std::pair<afterhours::input::DeviceMedium, float> check_action_pressed(
    int gamepad_id, const afterhours::input::ValidInputs& valid_inputs) {
    using Medium = afterhours::input::DeviceMedium;
    Medium medium = Medium::None;
    float value = 0.f;

    for (const auto& input : valid_inputs) {
        Medium temp_medium = Medium::None;
        float temp = 0.f;

        if (input.index() == 0) {
            temp_medium = Medium::Keyboard;
            temp = check_key_pressed(std::get<0>(input));
        } else if (input.index() == 1) {
            // For axes, "pressed" means crossing the deadzone threshold
            // We use the down check for now (no edge detection for axes)
            temp_medium = Medium::GamepadAxis;
            temp = check_axis(gamepad_id, std::get<1>(input));
        } else if (input.index() == 2) {
            temp_medium = Medium::GamepadButton;
            temp = check_button_pressed(gamepad_id, std::get<2>(input));
        }

        if (temp > value) {
            value = temp;
            medium = temp_medium;
        }
    }
    return {medium, value};
}

}  // namespace local_input

// Helper to build layer mapping from KeyMap
void build_layer_mapping(menu::State state, LayerMapping& layer_mapping) {
    for (auto name : magic_enum::enum_values<InputName>()) {
        const AnyInputs& inputs = KeyMap::get_valid_inputs(state, name);
        if (inputs.empty()) continue;

        afterhours::input::ValidInputs ah_inputs;
        for (const auto& input : inputs) {
            if (std::holds_alternative<int>(input)) {
                ah_inputs.push_back(std::get<int>(input));
            } else if (std::holds_alternative<GamepadAxisWithDir>(input)) {
                ah_inputs.push_back(
                    convert_axis(std::get<GamepadAxisWithDir>(input)));
            } else if (std::holds_alternative<raylib::GamepadButton>(input)) {
                ah_inputs.push_back(std::get<raylib::GamepadButton>(input));
            }
        }
        if (!ah_inputs.empty()) {
            layer_mapping[static_cast<int>(name)] = ah_inputs;
        }
    }
}

// Build mapping from KeyMap's default keys
void build_mapping_from_keymap() {
    // Force KeyMap to initialize (void cast to suppress nodiscard warning)
    (void) KeyMap::get();

    build_layer_mapping(menu::State::Game, g_mapping[menu::State::Game]);
    build_layer_mapping(menu::State::UI, g_mapping[menu::State::UI]);

    // Root inherits from UI
    g_mapping[menu::State::Root] = g_mapping[menu::State::UI];
}

// Fetch the max available gamepad ID
int fetch_max_gamepad_id() {
    int result = -1;
    for (int i = 0; i < afterhours::input::MAX_GAMEPAD_ID; i++) {
        if (!afterhours::input::is_gamepad_available(i)) {
            result = i - 1;
            break;
        }
    }
    return result;
}

// Poll all inputs for all layers
// We poll all layers so that consumption works regardless of which layer is
// checked
void poll_inputs(float dt) {
    g_max_gamepad = std::max(0, fetch_max_gamepad_id());
    g_collector.inputs.clear();
    g_collector.inputs_pressed.clear();

    // Poll all layers so consumption works for any layer
    for (const auto& [layer, layer_mapping] : g_mapping) {
        for (const auto& [action, valid_inputs] : layer_mapping) {
            for (int gamepad_id = 0; gamepad_id <= g_max_gamepad;
                 gamepad_id++) {
                // Check "down" state using local_input (supports synthetic
                // keys)
                {
                    const auto [down_medium, down_amount] =
                        local_input::check_action_down(gamepad_id,
                                                       valid_inputs);
                    if (down_amount > 0.f) {
                        // Only add if not already present (avoid duplicates
                        // from shared inputs)
                        bool already_present = false;
                        for (const auto& existing : g_collector.inputs) {
                            if (existing.action == action &&
                                existing.id == gamepad_id) {
                                already_present = true;
                                break;
                            }
                        }
                        if (!already_present) {
                            g_collector.inputs.push_back(
                                afterhours::input::ActionDone(
                                    down_medium, gamepad_id, action,
                                    down_amount, dt));
                        }
                    }
                }
                // Check "pressed" state (just this frame)
                {
                    const auto [pressed_medium, pressed_amount] =
                        local_input::check_action_pressed(gamepad_id,
                                                          valid_inputs);
                    if (pressed_amount > 0.f) {
                        // Only add if not already present (avoid duplicates
                        // from shared inputs)
                        bool already_present = false;
                        for (const auto& existing :
                             g_collector.inputs_pressed) {
                            if (existing.action == action &&
                                existing.id == gamepad_id) {
                                already_present = true;
                                break;
                            }
                        }
                        if (!already_present) {
                            g_collector.inputs_pressed.push_back(
                                afterhours::input::ActionDone(
                                    pressed_medium, gamepad_id, action,
                                    pressed_amount, dt));
                        }
                    }
                }
            }
        }
    }

    if (g_collector.inputs.empty()) {
        g_collector.since_last_input += dt;
    } else {
        g_collector.since_last_input = 0.f;
    }
}

}  // namespace

void init() {
    if (g_initialized) return;
    g_initialized = true;

    // Build mapping from KeyMap defaults
    build_mapping_from_keymap();
}

// Called each frame before input collection to poll raw input
void poll(float dt) { poll_inputs(dt); }

float is_down(InputName name) {
    for (const auto& action : g_collector.inputs) {
        if (action.action == static_cast<int>(name)) {
            return action.amount_pressed;
        }
    }
    return 0.f;
}

bool was_pressed(InputName name) {
    for (const auto& action : g_collector.inputs_pressed) {
        if (action.action == static_cast<int>(name)) {
            return true;
        }
    }
    return false;
}

std::vector<InputEvent> get_pressed_this_frame() {
    std::vector<InputEvent> events;
    for (const auto& action : g_collector.inputs_pressed) {
        events.push_back(InputEvent{
            .name = static_cast<InputName>(action.action),
            .amount = action.amount_pressed,
            .gamepad_id = action.id,
        });
    }
    return events;
}

void consume_pressed(InputName name) {
    // Remove all instances of this action from inputs_pressed
    auto& pressed = g_collector.inputs_pressed;
    pressed.erase(
        std::remove_if(pressed.begin(), pressed.end(),
                       [name](const afterhours::input::ActionDone& action) {
                           return action.action == static_cast<int>(name);
                       }),
        pressed.end());
}

}  // namespace input_helper
