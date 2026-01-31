// src/engine/input_helper.cpp
//
// Provides input abstraction using afterhours layered input system.
// Supports different key bindings per menu::State (Game vs UI).
//
// NOTE: We implement our own input checking instead of using afterhours'
// check_single_action_down/pressed functions because we need to use
// pharmasea's ext::is_key_down/is_key_pressed wrappers for replay
// system compatibility (synthetic key injection).
//

#include "input_helper.h"

#include "afterhours/src/plugins/input_system.h"
#include "graphics.h"  // For ext::is_key_down, ext::is_key_pressed
#include "keymap.h"
#include "log/log.h"
#include "magic_enum/magic_enum.hpp"
#include "statemanager.h"

namespace input_helper {

namespace {

// Static storage for layered input
using LayerMapping = std::map<int, afterhours::input::ValidInputs>;
using FullMapping = std::map<menu::State, LayerMapping>;

FullMapping g_mapping;
menu::State g_active_layer = menu::State::Root;

// Input state for current frame
afterhours::input::InputCollector g_collector;
int g_max_gamepad = 0;
bool g_initialized = false;

// Internal helper to map menu state to input layer
menu::State get_input_layer(menu::State state) {
    if (state == menu::State::Game) {
        return menu::State::Game;
    }
    return menu::State::UI;
}

// Convert pharmasea GamepadAxisWithDir to afterhours format
afterhours::input::GamepadAxisWithDir convert_axis(const GamepadAxisWithDir& src) {
    return afterhours::input::GamepadAxisWithDir{
        .axis = src.axis,
        .dir = static_cast<int>(src.dir)
    };
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

float check_axis(int gamepad_id, const afterhours::input::GamepadAxisWithDir& axis_with_dir) {
    const float mvt = raylib::GetGamepadAxisMovement(gamepad_id, axis_with_dir.axis);
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

// Build mapping from KeyMap's default keys
void build_mapping_from_keymap() {
    // Force KeyMap to initialize (void cast to suppress nodiscard warning)
    (void)KeyMap::get();

    // We need to extract the mapping from KeyMap
    // For each layer and input, get the valid inputs and convert

    // Game layer
    LayerMapping& game = g_mapping[menu::State::Game];
    for (int i = 0; i <= static_cast<int>(InputName::SkipIngredientMatch); i++) {
        InputName name = static_cast<InputName>(i);
        const AnyInputs& inputs = KeyMap::get_valid_inputs(menu::State::Game, name);
        if (inputs.empty()) continue;

        afterhours::input::ValidInputs ah_inputs;
        for (const auto& input : inputs) {
            if (std::holds_alternative<int>(input)) {
                ah_inputs.push_back(std::get<int>(input));
            } else if (std::holds_alternative<GamepadAxisWithDir>(input)) {
                ah_inputs.push_back(convert_axis(std::get<GamepadAxisWithDir>(input)));
            } else if (std::holds_alternative<raylib::GamepadButton>(input)) {
                ah_inputs.push_back(std::get<raylib::GamepadButton>(input));
            }
        }
        if (!ah_inputs.empty()) {
            game[i] = ah_inputs;
        }
    }

    // UI layer
    LayerMapping& ui = g_mapping[menu::State::UI];
    for (int i = 0; i <= static_cast<int>(InputName::SkipIngredientMatch); i++) {
        InputName name = static_cast<InputName>(i);
        const AnyInputs& inputs = KeyMap::get_valid_inputs(menu::State::UI, name);
        if (inputs.empty()) continue;

        afterhours::input::ValidInputs ah_inputs;
        for (const auto& input : inputs) {
            if (std::holds_alternative<int>(input)) {
                ah_inputs.push_back(std::get<int>(input));
            } else if (std::holds_alternative<GamepadAxisWithDir>(input)) {
                ah_inputs.push_back(convert_axis(std::get<GamepadAxisWithDir>(input)));
            } else if (std::holds_alternative<raylib::GamepadButton>(input)) {
                ah_inputs.push_back(std::get<raylib::GamepadButton>(input));
            }
        }
        if (!ah_inputs.empty()) {
            ui[i] = ah_inputs;
        }
    }

    // Root inherits from UI
    g_mapping[menu::State::Root] = ui;
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

// Poll all inputs for the active layer
void poll_inputs(float dt) {
    g_max_gamepad = std::max(0, fetch_max_gamepad_id());
    g_collector.inputs.clear();
    g_collector.inputs_pressed.clear();

    // Get the active layer's mapping
    auto layer_it = g_mapping.find(g_active_layer);
    if (layer_it == g_mapping.end()) {
        if (g_collector.inputs.empty()) {
            g_collector.since_last_input += dt;
        }
        return;
    }

    for (const auto& [action, valid_inputs] : layer_it->second) {
        for (int gamepad_id = 0; gamepad_id <= g_max_gamepad; gamepad_id++) {
            // Check "down" state using local_input (supports synthetic keys)
            {
                const auto [down_medium, down_amount] =
                    local_input::check_action_down(gamepad_id, valid_inputs);
                if (down_amount > 0.f) {
                    g_collector.inputs.push_back(
                        afterhours::input::ActionDone(down_medium, gamepad_id, action, down_amount, dt));
                }
            }
            // Check "pressed" state (just this frame)
            {
                const auto [pressed_medium, pressed_amount] =
                    local_input::check_action_pressed(gamepad_id, valid_inputs);
                if (pressed_amount > 0.f) {
                    g_collector.inputs_pressed.push_back(
                        afterhours::input::ActionDone(pressed_medium, gamepad_id, action, pressed_amount, dt));
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

    // Set initial active layer
    g_active_layer = get_input_layer(MenuState::get().read());

    // Register layer sync callback
    MenuState::get().register_on_change(
        [](menu::State new_state, menu::State /*old_state*/) {
            g_active_layer = get_input_layer(new_state);
        });
}

void sync_layer() {
    g_active_layer = get_input_layer(MenuState::get().read());
}

// Called each frame before input collection to poll raw input
void poll(float dt) {
    poll_inputs(dt);
}

float is_down_for_layer(menu::State layer, InputName name) {
    // Check the collector for this action in the requested layer
    if (layer == g_active_layer) {
        for (const auto& action : g_collector.inputs) {
            if (action.action == static_cast<int>(name)) {
                return action.amount_pressed;
            }
        }
        return 0.f;
    }
    // For non-active layer, check input directly from mapping
    // Note: This is expected behavior when explicitly querying a specific layer
    log_trace("Fetching key {} for non-active layer {}",
             magic_enum::enum_name(name), magic_enum::enum_name(layer));
    auto layer_it = g_mapping.find(layer);
    if (layer_it == g_mapping.end()) {
        return 0.f;
    }
    auto action_it = layer_it->second.find(static_cast<int>(name));
    if (action_it == layer_it->second.end()) {
        return 0.f;
    }
    // Check input directly using our local functions (supports synthetic keys)
    for (int gamepad_id = 0; gamepad_id <= g_max_gamepad; gamepad_id++) {
        const auto [medium, amount] = local_input::check_action_down(gamepad_id, action_it->second);
        if (amount > 0.f) {
            return amount;
        }
    }
    return 0.f;
}

float is_down(InputName name) {
    menu::State state = MenuState::get().read();
    return is_down_for_layer(get_input_layer(state), name);
}

bool was_pressed_for_layer(menu::State layer, InputName name) {
    if (layer == g_active_layer) {
        for (const auto& action : g_collector.inputs_pressed) {
            if (action.action == static_cast<int>(name)) {
                return true;
            }
        }
        return false;
    }
    // For non-active layer, check input directly from mapping
    // Note: This is expected behavior when explicitly querying a specific layer
    log_trace("Fetching key {} for non-active layer {}",
             magic_enum::enum_name(name), magic_enum::enum_name(layer));
    auto layer_it = g_mapping.find(layer);
    if (layer_it == g_mapping.end()) {
        return false;
    }
    auto action_it = layer_it->second.find(static_cast<int>(name));
    if (action_it == layer_it->second.end()) {
        return false;
    }
    // Check input directly using our local functions (supports synthetic keys)
    for (int gamepad_id = 0; gamepad_id <= g_max_gamepad; gamepad_id++) {
        const auto [medium, amount] = local_input::check_action_pressed(gamepad_id, action_it->second);
        if (amount > 0.f) {
            return true;
        }
    }
    return false;
}

bool was_pressed(InputName name) {
    menu::State state = MenuState::get().read();
    return was_pressed_for_layer(get_input_layer(state), name);
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

}  // namespace input_helper
