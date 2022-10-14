
#pragma once

#include "external_include.h"
//
#include "assert.h"
#include "event.h"
#include "keymap.h"
#include "menu.h"
#include "raylib.h"
#include "vec_util.h"
//
#include "ui_color.h"
#include "ui_state.h"
#include "uuid.h"

namespace ui {
const Menu::State STATE = Menu::State::UI;

struct UIContext;
static std::shared_ptr<UIContext> _uicontext;
UIContext* globalContext;

struct UIContext {
    StateManager statemanager;

    uuid hot_id;
    uuid active_id;
    uuid kb_focus_id;
    uuid last_processed;

    bool lmouse_down = false;
    vec2 mouse = vec2{0.0, 0.0};

    int key = -1;
    int mod = -1;
    GamepadButton button;

    inline static UIContext* create() { return new UIContext(); }
    inline static UIContext& get() {
        if (globalContext) return *globalContext;
        if (!_uicontext) _uicontext.reset(UIContext::create());
        return *_uicontext;
    }

    bool is_mouse_inside(const Rectangle& rect) {
        return mouse.x >= rect.x && mouse.x <= rect.x + rect.width &&
               mouse.y >= rect.y && mouse.y <= rect.y + rect.height;
    }

    bool process_keyevent(KeyPressedEvent event) {
        int code = event.keycode;
        if (!KeyMap::does_layer_map_contain_key(STATE, code)) {
            return false;
        }
        // TODO make this a map if we have more
        if (code == KeyMap::get_key_code(STATE, "Widget Mod")) {
            mod = code;
            return true;
        }
        key = code;
        return true;
    }

    bool process_gamepad_button_event(GamepadButtonPressedEvent event) {
        GamepadButton code = event.button;
        if (!KeyMap::does_layer_map_contain_button(STATE, code)) {
            return false;
        }
        button = code;
        return true;
    }

    bool _pressedButtonWithoutEat(GamepadButton butt) const {
        if (butt == GAMEPAD_BUTTON_UNKNOWN) return false;
        return button == butt;
    }

    bool pressedButtonWithoutEat(std::string name) const {
        GamepadButton code = KeyMap::get_button(STATE, name);
        return _pressedWithoutEat(code);
    }

    void eatButton() { 
        button = GAMEPAD_BUTTON_UNKNOWN;
    }

    bool pressed(std::string name) {
        int code = KeyMap::get_key_code(STATE, name);
        bool a = _pressedWithoutEat(code);
        if (a) {
            eatKey();
            return a;
        }

        return a;

        GamepadButton butt = KeyMap::get_button(STATE, name);
        bool b = _pressedButtonWithoutEat(butt);
        if (b) {
            eatButton();
        }
        return b;
    }

    bool _pressedWithoutEat(int code) const {
        if (code == KEY_NULL) return false;
        return key == code || mod == code;
    }
    // TODO is there a better way to do eat(string)?
    bool pressedWithoutEat(std::string name) const {
        int code = KeyMap::get_key_code(STATE, name);
        return _pressedWithoutEat(code);
    }

    void eatKey() { key = int(); }

    // is held down
    bool is_held_down(std::string name) {
        // TODO
        return KeyMap::is_event(STATE, name);
    }

    void draw_widget(vec2 pos, vec2 size, float, Color color, std::string) {
        Rectangle rect = {pos.x, pos.y, size.x, size.y};
        DrawRectangleRounded(rect, 0.15f, 4, color);
    }

    inline vec2 widget_center(vec2 position, vec2 size) {
        return position + (size / 2.f);
    }

    //
    bool inited = false;
    bool began_and_not_ended = false;

    void init() {
        inited = true;
        began_and_not_ended = false;

        hot_id = ROOT_ID;
        active_id = ROOT_ID;
        kb_focus_id = ROOT_ID;

        lmouse_down = false;
        mouse = vec2{};
    }

    void begin(bool mouseDown, const vec2& mousePos) {
        M_ASSERT(inited, "UIContext must be inited before you begin()");
        M_ASSERT(!began_and_not_ended,
                 "You should call end every frame before calling begin() "
                 "again ");
        began_and_not_ended = true;

        globalContext = this;
        hot_id = ROOT_ID;
        lmouse_down = mouseDown;
        mouse = mousePos;
    }

    void end() {
        began_and_not_ended = false;
        if (lmouse_down) {
            if (active_id == ROOT_ID) {
                active_id = FAKE_ID;
            }
        } else {
            active_id = ROOT_ID;
        }
        // key = int();
        // mod = int();
        // button = GAMEPAD_BUTTON_UNKNOWN;
        //
        // keychar = int();
        // modchar = int();
        globalContext = nullptr;
    }

    template<typename T>
    std::shared_ptr<T> widget_init(const uuid id) {
        std::shared_ptr<T> state = statemanager.getAndCreateIfNone<T>(id);
        if (state == nullptr) {
            // TODO add log support
            // log_error(
            // "State for id ({}) of wrong type, expected {}. Check to "
            // "make sure your id's are globally unique",
            // std::string(id), type_name<T>());
        }
        return state;
    }
};

UIContext& get() { return UIContext::get(); }

}  // namespace ui
