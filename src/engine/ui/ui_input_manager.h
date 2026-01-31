
#pragma once
#include "../../vendor_include.h"
#include "../input_helper.h"
#include "../input_utilities.h"
#include "../keymap.h"
#include "sound.h"

namespace ui {

namespace detail {
inline bool layer_contains_key(menu::State state, int keycode) {
    for (auto name : magic_enum::enum_values<InputName>()) {
        if (afterhours::input_ext::contains_key(KeyMap::get_valid_inputs(state, name), keycode)) {
            return true;
        }
    }
    return false;
}

inline bool layer_contains_button(menu::State state, GamepadButton button) {
    for (auto name : magic_enum::enum_values<InputName>()) {
        if (afterhours::input_ext::contains_button(KeyMap::get_valid_inputs(state, name), button)) {
            return true;
        }
    }
    return false;
}

inline bool layer_contains_axis(menu::State state, GamepadAxis axis) {
    for (auto name : magic_enum::enum_values<InputName>()) {
        if (afterhours::input_ext::contains_axis(KeyMap::get_valid_inputs(state, name), axis)) {
            return true;
        }
    }
    return false;
}
}  // namespace detail

struct IUIContextInputManager {
    const menu::State STATE = menu::State::UI;

    virtual ~IUIContextInputManager() {}

    virtual void init() {
        mouse_info = MouseInfo();
        yscrolled = 0.f;
    }

    virtual void begin(float dt) {
        mouse_info = get_mouse_info();
        // TODO Should this be more like mousePos?
        yscrolled = ext::get_mouse_wheel_move();

        lastDt = dt;
    }

    virtual void cleanup() {
        // button = raylib::GAMEPAD_BUTTON_UNKNOWN;
        //
        key = int();
        mod = int();

        keychar = int();
        modchar = int();

        // Note: if you stop doing any keyhelds for a bit, then we
        // reset you so the next key press is faster
        if (keyHeldDownTimer == prevKeyHeldDownTimer) {
            keyHeldDownTimer = 0.f;
        }
        prevKeyHeldDownTimer = keyHeldDownTimer;
    }

    AnyInput anything_pressed = 0;

    AnyInput last_input_pressed() { return anything_pressed; }
    void clear_last_input() { anything_pressed = 0; }

    MouseInfo mouse_info;

    int key = -1;
    int mod = -1;
    GamepadButton button;
    GamepadAxisWithDir axis_info;
    int keychar = -1;
    int modchar = -1;
    float yscrolled;
    // TODO can we use timer stuff for this? we have tons of these around
    // the codebase
    float keyHeldDownTimer = 0.f;
    float prevKeyHeldDownTimer = 0.f;
    float keyHeldDownTimerReset = 0.15f;
    float lastDt;

    [[nodiscard]] bool is_mouse_inside(const Rectangle& rect) const {
        auto mouse = mouse_info.pos;
        return mouse.x >= rect.x && mouse.x <= rect.x + rect.width &&
               mouse.y >= rect.y && mouse.y <= rect.y + rect.height;
    }

    [[nodiscard]] bool process_char_press_event(const CharPressedEvent& event) {
        keychar = event.keycode;
        return true;
    }

    [[nodiscard]] bool process_keyevent(const KeyPressedEvent& event) {
        int code = event.keycode;
        anything_pressed = code;
        if (!detail::layer_contains_key(STATE, code)) {
            return false;
        }
        auto widget_mod_key = afterhours::input_ext::get_first_key(
            KeyMap::get_valid_inputs(STATE, InputName::WidgetMod));
        if (widget_mod_key.has_value() && code == widget_mod_key.value()) {
            mod = code;
            return true;
        }
        auto widget_ctrl_key = afterhours::input_ext::get_first_key(
            KeyMap::get_valid_inputs(STATE, InputName::WidgetCtrl));
        if (widget_ctrl_key.has_value() && code == widget_ctrl_key.value()) {
            mod = code;
            return true;
        }

        // TODO same as above, but a separate map
        modchar = code;

        key = code;
        return true;
    }

    [[nodiscard]] bool process_gamepad_button_event(
        const GamepadButtonPressedEvent& event) {
        GamepadButton code = event.button;
        anything_pressed = code;
        if (!detail::layer_contains_button(STATE, code)) {
            return false;
        }
        button = code;
        return true;
    }

    [[nodiscard]] bool process_gamepad_axis_event(GamepadAxisMovedEvent event) {
        GamepadAxisWithDir info = event.data;
        anything_pressed = info;
        if (!detail::layer_contains_axis(STATE, info.axis)) {
            return false;
        }
        axis_info = info;
        return true;
    }

    [[nodiscard]] bool _pressedButtonWithoutEat(GamepadButton butt) const {
        if (butt == raylib::GAMEPAD_BUTTON_UNKNOWN) return false;
        return button == butt;
    }

    [[nodiscard]] bool pressedButtonWithoutEat(const InputName& name) const {
        auto code = afterhours::input_ext::get_first_button(
            KeyMap::get_valid_inputs(STATE, name));
        if (!code.has_value()) return false;
        return _pressedButtonWithoutEat(code.value());
    }

    void eatButton() { button = raylib::GAMEPAD_BUTTON_UNKNOWN; }

    [[nodiscard]] bool pressed(const InputName& name) {
        auto key_opt = afterhours::input_ext::get_first_key(
            KeyMap::get_valid_inputs(STATE, name));
        if (key_opt.has_value()) {
            bool a = _pressedWithoutEat(key_opt.value());
            if (a) {
                ui::sounds::select();
                eatKey();
                return a;
            }
        }

        auto butt = afterhours::input_ext::get_first_button(
            KeyMap::get_valid_inputs(STATE, name));
        if (butt.has_value()) {
            bool b = _pressedButtonWithoutEat(butt.value());
            if (b) {
                ui::sounds::select();
                eatButton();
                return b;
            }
        }

        auto axis_opt = afterhours::input_ext::get_first_axis(
            KeyMap::get_valid_inputs(STATE, name));
        if (axis_opt.has_value()) {
            auto axis = axis_opt.value();
            bool c = axis_info.axis == axis.axis &&
                     ((axis.dir - axis_info.dir) >= EPSILON);
            if (c) {
                ui::sounds::select();
                eatAxis();
                return c;
            }
        }
        return false;
    }

    void handleBadGamepadAxis(const KeyMapInputRequestError&, menu::State,
                              const InputName) {
        // TODO theres currently no valid inputs for axis on UI items so this is
        // all just firing constantly. log_warn("{}: No gamepad axis in {} for
        // {}", err, state, magic_enum::enum_name(name));
    }

    void eatAxis() { axis_info = {}; }

    [[nodiscard]] bool _pressedWithoutEat(int code) const {
        if (code == raylib::KEY_NULL) return false;
        return key == code || mod == code;
    }
    // TODO is there a better way to do eat(string)?
    [[nodiscard]] bool pressedWithoutEat(const InputName& name) const {
        auto code = afterhours::input_ext::get_first_key(
            KeyMap::get_valid_inputs(STATE, name));
        if (!code.has_value()) return false;
        return _pressedWithoutEat(code.value());
    }

    void eatKey() { key = int(); }

    [[nodiscard]] bool is_held_down_debounced(const InputName& name) {
        const bool is_held = is_held_down(name);
        if (keyHeldDownTimer < keyHeldDownTimerReset) {
            keyHeldDownTimer += lastDt;
            return false;
        }
        keyHeldDownTimer = 0.f;
        return is_held;
    }

    [[nodiscard]] bool is_held_down(const InputName& name) {
        return input_helper::is_down(name) > 0.f;
    }
};
}  // namespace ui
