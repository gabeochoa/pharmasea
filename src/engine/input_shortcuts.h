// src/engine/input_shortcuts.h
//
// Convenience wrappers that combine KeyMap::get_valid_inputs with
// input_utilities. Provides a shorter API for common input checking patterns.

#pragma once

#include "input_utilities.h"
#include "keymap.h"

namespace input {

inline bool contains_key(menu::State state, InputName name, int keycode) {
    return afterhours::input_ext::contains_key(
        KeyMap::get_valid_inputs(state, name), keycode);
}

inline bool contains_button(menu::State state, InputName name,
                            GamepadButton button) {
    return afterhours::input_ext::contains_button(
        KeyMap::get_valid_inputs(state, name), button);
}

inline bool contains_axis(menu::State state, InputName name,
                          raylib::GamepadAxis axis) {
    return afterhours::input_ext::contains_axis(
        KeyMap::get_valid_inputs(state, name), axis);
}

template<typename... Pairs>
inline bool contains_key_in_any(int keycode, Pairs&&... pairs) {
    return (contains_key(pairs.first, pairs.second, keycode) || ...);
}

template<typename... Pairs>
inline bool contains_button_in_any(GamepadButton button, Pairs&&... pairs) {
    return (contains_button(pairs.first, pairs.second, button) || ...);
}

inline std::optional<int> get_first_key(menu::State state, InputName name) {
    return afterhours::input_ext::get_first_key(
        KeyMap::get_valid_inputs(state, name));
}

inline std::optional<GamepadButton> get_first_button(menu::State state,
                                                     InputName name) {
    return afterhours::input_ext::get_first_button(
        KeyMap::get_valid_inputs(state, name));
}

inline auto get_first_axis(menu::State state, InputName name) {
    return afterhours::input_ext::get_first_axis(
        KeyMap::get_valid_inputs(state, name));
}

}  // namespace input
