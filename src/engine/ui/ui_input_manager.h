
#pragma once
#include "../../game_actions.h"
#include "../../vendor_include.h"
#include "../input_helper.h"
#include "../input_utilities.h"
#include "../keymap.h"
#include "../log.h"
#include "afterhours/src/plugins/input_system.h"
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
        poll_input();
    }

    void poll_input() {
        // Reset per-frame state
        chars_this_frame.clear();

        // Buffer all characters pressed this frame (for text fields)
        int ch = afterhours::input::get_char_pressed();
        while (ch != 0) {
            chars_this_frame += static_cast<char>(ch);
            ch = afterhours::input::get_char_pressed();
        }
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
    int keychar = -1;  // Deprecated: use chars_this_frame
    std::string chars_this_frame;
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

    // Backwards-compatible getter for keychar
    [[nodiscard]] int get_keychar() const {
        return chars_this_frame.empty() ? 0
                                        : static_cast<int>(chars_this_frame[0]);
    }

    [[nodiscard]] const std::string& get_chars_this_frame() const {
        return chars_this_frame;
    }

    // Polling-based input methods that use GameAction
    [[nodiscard]] bool pressed(game::GameAction action) {
        auto* collector =
            afterhours::EntityHelper::get_singleton_cmp<afterhours::input::InputCollector>();
        if (!collector) return false;

        for (const auto& a : collector->inputs_pressed) {
            if (a.action == static_cast<int>(action)) {
                ui::sounds::select();
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] bool is_held_down(game::GameAction action) {
        auto* collector =
            afterhours::EntityHelper::get_singleton_cmp<afterhours::input::InputCollector>();
        if (!collector) return false;

        for (const auto& a : collector->inputs) {
            if (a.action == static_cast<int>(action)) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] bool is_held_down_debounced(game::GameAction action) {
        const bool is_held = is_held_down(action);
        if (!is_held) return false;
        if (keyHeldDownTimer < keyHeldDownTimerReset) {
            keyHeldDownTimer += lastDt;
            return false;
        }
        keyHeldDownTimer = 0.f;
        return true;
    }

    [[nodiscard]] bool _pressedButtonWithoutEat(GamepadButton butt) const {
        if (butt == raylib::GAMEPAD_BUTTON_UNKNOWN) return false;
        return afterhours::input::is_gamepad_button_pressed(0, butt);
    }

    [[nodiscard]] bool pressedButtonWithoutEat(const InputName& name) const {
        return input_helper::was_pressed(name);
    }

    void eatButton() { button = raylib::GAMEPAD_BUTTON_UNKNOWN; }

    [[nodiscard]] bool pressed(const InputName& name) {
        bool result = input_helper::was_pressed(name);
        if (result) {
            ui::sounds::select();
        }
        return result;
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
        return afterhours::input::is_key_pressed(code);
    }
    // TODO is there a better way to do eat(string)?
    [[nodiscard]] bool pressedWithoutEat(const InputName& name) const {
        return input_helper::was_pressed(name);
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
