// src/engine/input_utilities.h
//
// Self-contained input utilities module using afterhours types.
// Designed to be portable to afterhours as a plugin once stable.
//

#pragma once

#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "afterhours/src/plugins/input_system.h"
#include "gamepad_axis_with_dir.h"
#include "magic_enum/magic_enum.hpp"

namespace afterhours::input_ext {

// Missing from afterhours::input (only has icon_for_key)
inline std::string name_for_key(int keycode) {
    auto key = magic_enum::enum_cast<raylib::KeyboardKey>(
        static_cast<unsigned int>(keycode));
    if (key.has_value()) {
        return std::string(magic_enum::enum_name(key.value()));
    }
    return "Unknown";
}

// Missing from afterhours::input - works with any GamepadAxisWithDir-like type
template<typename AxisT>
inline std::string name_for_axis(const AxisT& axis) {
    std::string dir_str = (axis.dir > 0) ? "+" : "-";
    auto axis_name = magic_enum::enum_name(axis.axis);
    return std::string(axis_name) + " " + dir_str;
}

// Missing from afterhours::input - works with any GamepadAxisWithDir-like type
template<typename AxisT>
inline std::string icon_for_axis(const AxisT& axis) {
    std::string dir_str = (axis.dir > 0) ? "+" : "-";
    auto axis_name = magic_enum::enum_name(axis.axis);
    return "axis_" + std::string(axis_name) + "_" + dir_str;
}

namespace detail {
template<typename T>
struct is_variant : std::false_type {};
template<typename... Ts>
struct is_variant<std::variant<Ts...>> : std::true_type {};
}  // namespace detail

// name_for_input - overload for int (keyboard key)
inline std::string name_for_input(int keycode) { return name_for_key(keycode); }

// name_for_input - overload for button
inline std::string name_for_input(raylib::GamepadButton button) {
    return afterhours::input::name_for_button(button);
}

// name_for_input - overload for variant types (dispatches via std::visit)
template<typename VariantT,
         std::enable_if_t<detail::is_variant<std::decay_t<VariantT>>::value,
                          int> = 0>
inline std::string name_for_input(const VariantT& input) {
    return std::visit(
        [](const auto& val) -> std::string {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, int>) {
                return name_for_key(val);
            } else if constexpr (std::is_same_v<T, raylib::GamepadButton>) {
                return afterhours::input::name_for_button(val);
            } else {
                return name_for_axis(val);
            }
        },
        input);
}

// icon_for_input - overload for int (keyboard key)
inline std::string icon_for_input(int keycode) {
    return afterhours::input::icon_for_key(keycode);
}

// icon_for_input - overload for button
inline std::string icon_for_input(raylib::GamepadButton button) {
    return afterhours::input::icon_for_button(button);
}

// icon_for_input - overload for variant types (dispatches via std::visit)
template<typename VariantT,
         std::enable_if_t<detail::is_variant<std::decay_t<VariantT>>::value,
                          int> = 0>
inline std::string icon_for_input(const VariantT& input) {
    return std::visit(
        [](const auto& val) -> std::string {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, int>) {
                return afterhours::input::icon_for_key(val);
            } else if constexpr (std::is_same_v<T, raylib::GamepadButton>) {
                return afterhours::input::icon_for_button(val);
            } else {
                return icon_for_axis(val);
            }
        },
        input);
}

// Query functions - missing from afterhours::input
template<typename ValidInputsT>
inline std::optional<int> get_first_key(const ValidInputsT& inputs) {
    for (const auto& input : inputs) {
        if (std::holds_alternative<int>(input)) {
            return std::get<int>(input);
        }
    }
    return std::nullopt;
}

template<typename ValidInputsT>
inline std::optional<raylib::GamepadButton> get_first_button(
    const ValidInputsT& inputs) {
    for (const auto& input : inputs) {
        if (std::holds_alternative<raylib::GamepadButton>(input)) {
            return std::get<raylib::GamepadButton>(input);
        }
    }
    return std::nullopt;
}

template<typename ValidInputsT, typename AxisT = ::GamepadAxisWithDir>
inline std::optional<AxisT> get_first_axis(const ValidInputsT& inputs) {
    for (const auto& input : inputs) {
        if (std::holds_alternative<AxisT>(input)) {
            return std::get<AxisT>(input);
        }
    }
    return std::nullopt;
}

// Reverse lookup - missing from afterhours::input
template<typename ValidInputsT>
inline bool contains_key(const ValidInputsT& inputs, int keycode) {
    for (const auto& input : inputs) {
        if (std::holds_alternative<int>(input) &&
            std::get<int>(input) == keycode) {
            return true;
        }
    }
    return false;
}

template<typename ValidInputsT>
inline bool contains_button(const ValidInputsT& inputs,
                            raylib::GamepadButton button) {
    for (const auto& input : inputs) {
        if (std::holds_alternative<raylib::GamepadButton>(input) &&
            std::get<raylib::GamepadButton>(input) == button) {
            return true;
        }
    }
    return false;
}

template<typename ValidInputsT, typename AxisT = ::GamepadAxisWithDir>
inline bool contains_axis(const ValidInputsT& inputs,
                          raylib::GamepadAxis axis) {
    for (const auto& input : inputs) {
        if (std::holds_alternative<AxisT>(input) &&
            std::get<AxisT>(input).axis == axis) {
            return true;
        }
    }
    return false;
}

template<typename... ValidInputsT>
inline bool contains_key_in_any(int keycode,
                                const ValidInputsT&... inputs_list) {
    return (contains_key(inputs_list, keycode) || ...);
}

template<typename... ValidInputsT>
inline bool contains_button_in_any(raylib::GamepadButton button,
                                   const ValidInputsT&... inputs_list) {
    return (contains_button(inputs_list, button) || ...);
}

// Extract all keyboard keys from a ValidInputs collection
template<typename ValidInputsT>
inline std::vector<int> get_all_keys(const ValidInputsT& inputs) {
    std::vector<int> keys;
    for (const auto& input : inputs) {
        if (std::holds_alternative<int>(input)) {
            keys.push_back(std::get<int>(input));
        }
    }
    return keys;
}

// Extract all gamepad buttons from a ValidInputs collection
template<typename ValidInputsT>
inline std::vector<raylib::GamepadButton> get_all_buttons(
    const ValidInputsT& inputs) {
    std::vector<raylib::GamepadButton> buttons;
    for (const auto& input : inputs) {
        if (std::holds_alternative<raylib::GamepadButton>(input)) {
            buttons.push_back(std::get<raylib::GamepadButton>(input));
        }
    }
    return buttons;
}

// Polling functions - check if any input in a collection is currently
// pressed/down These are the polling alternatives to event-based input handling

// Check if any key in the valid inputs is just pressed this frame
template<typename ValidInputsT>
inline bool is_any_key_just_pressed(const ValidInputsT& inputs) {
    for (const auto& input : inputs) {
        if (std::holds_alternative<int>(input)) {
            if (afterhours::input::is_key_pressed(std::get<int>(input))) {
                return true;
            }
        }
    }
    return false;
}

// Check if any key in the valid inputs is currently held down
template<typename ValidInputsT>
inline bool is_any_key_down(const ValidInputsT& inputs) {
    for (const auto& input : inputs) {
        if (std::holds_alternative<int>(input)) {
            if (afterhours::input::is_key_down(std::get<int>(input))) {
                return true;
            }
        }
    }
    return false;
}

// Check if any gamepad button in the valid inputs is just pressed this frame
template<typename ValidInputsT>
inline bool is_any_button_just_pressed(const ValidInputsT& inputs,
                                       int gamepad_id = 0) {
    for (const auto& input : inputs) {
        if (std::holds_alternative<raylib::GamepadButton>(input)) {
            if (afterhours::input::is_gamepad_button_pressed(
                    gamepad_id, std::get<raylib::GamepadButton>(input))) {
                return true;
            }
        }
    }
    return false;
}

// Check if any gamepad button in the valid inputs is currently held down
template<typename ValidInputsT>
inline bool is_any_button_down(const ValidInputsT& inputs, int gamepad_id = 0) {
    for (const auto& input : inputs) {
        if (std::holds_alternative<raylib::GamepadButton>(input)) {
            if (afterhours::input::is_gamepad_button_down(
                    gamepad_id, std::get<raylib::GamepadButton>(input))) {
                return true;
            }
        }
    }
    return false;
}

// Check if any input (key or button) in the valid inputs is just pressed
template<typename ValidInputsT>
inline bool is_any_input_just_pressed(const ValidInputsT& inputs,
                                      int gamepad_id = 0) {
    return is_any_key_just_pressed(inputs) ||
           is_any_button_just_pressed(inputs, gamepad_id);
}

// Check if any input (key or button) in the valid inputs is currently down
template<typename ValidInputsT>
inline bool is_any_input_down(const ValidInputsT& inputs, int gamepad_id = 0) {
    return is_any_key_down(inputs) || is_any_button_down(inputs, gamepad_id);
}

}  // namespace afterhours::input_ext
