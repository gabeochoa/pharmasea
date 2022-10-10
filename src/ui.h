
#pragma once

#include "assert.h"
#include "event.h"
#include "external_include.h"
#include "keymap.h"
#include "raylib.h"
#include "vec_util.h"
//
#include "ui_color.h"
#include "ui_state.h"
#include "ui_widget_config.h"
#include "uuid.h"

namespace ui {

// TODO add more info about begin()/end()

bool text(
    // returns true always
    const uuid,
    //
    const WidgetConfig&);

/* button
       returns true if the button was clicked else false

    button_with_label
        returns true if the button was clicked else false
        if config has .text will draw text directly through button() call
        if not then will check config.child for a separate text config
*/
bool button(const uuid id, WidgetConfig config);
bool button_with_label(const uuid id, WidgetConfig config);

// TODO is this what we want?
bool button_list(
    // returns true if any button pressed
    const uuid id,
    //
    WidgetConfig config,
    // List of text configs that should show in the dropdown
    const std::vector<WidgetConfig>& configs,
    // which item is selected
    int* selectedIndex = nullptr,
    //
    bool* hasFocus = nullptr);
//
bool dropdown(
    // returns true if dropdown value was changed
    const uuid id,
    // config
    WidgetConfig config,
    // List of text configs that should show in the dropdown
    const std::vector<WidgetConfig>& configs,
    // whether or not the dropdown is open
    bool* dropdownState,
    // which item is selected
    int* selectedIndex);

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
    vec2 mouse = vec2{0.0,0.0};

    int key = -1;
    int mod = -1;

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

    bool pressed(std::string name) {
        int code = KeyMap::get_key_code(STATE, name);
        bool a = _pressedWithoutEat(code);
        if (a) eatKey();
        return a;
    }
    bool _pressedWithoutEat(int code) const {
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
        DrawRectangle(static_cast<int>(pos.x), static_cast<int>(pos.y),    //
            static_cast<int>(size.x), static_cast<int>(size.y),  //
                      color);
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

inline bool is_hot(const uuid& id) { return (get().hot_id == id); }
inline bool is_active(const uuid& id) { return (get().active_id == id); }
inline bool is_active_or_hot(const uuid& id) {
    return is_hot(id) || is_active(id);
}
inline bool is_active_and_hot(const uuid& id) {
    return is_hot(id) && is_active(id);
}

inline void active_if_mouse_inside(const uuid id, const Rectangle& rect) {
    bool inside = get().is_mouse_inside(rect);
    if (inside) {
        get().hot_id = id;
        if (get().active_id == ROOT_ID && get().lmouse_down) {
            get().active_id = id;
        }
    }
    return;
}

inline void try_to_grab_kb(const uuid id) {
    if (get().kb_focus_id == ROOT_ID) {
        get().kb_focus_id = id;
    }
}

inline bool has_kb_focus(const uuid& id) { return (get().kb_focus_id == id); }
inline void draw_if_kb_focus(const uuid& id, std::function<void(void)> cb) {
    if (has_kb_focus(id)) cb();
}

inline void handle_tabbing(const uuid id) {
    // TODO How do we handle something that wants to use
    // Widget Value Down/Up to control the value?
    // Do we mark the widget type with "nextable"? (tab will always work but
    // not very discoverable
    if (has_kb_focus(id)) {
        if (get().pressed("Widget Next") ||
            get().pressed("Widget Value Down")) {
            get().kb_focus_id = ROOT_ID;
            if (get().is_held_down("Widget Mod")) {
                get().kb_focus_id = get().last_processed;
            }
        }
        if (get().pressed("Widget Value Up")) {
            get().kb_focus_id = get().last_processed;
        }
    }
    // before any returns
    get().last_processed = id;
}

bool _text_impl(const uuid id, const WidgetConfig& config) {
    // NOTE: currently id is only used for focus and hot/active,
    // we could potentially also track "selections"
    // with a range so the user can highlight text
    // not needed for supermarket but could be in the future?
    (void) id;
    // No need to render if text is empty
    if (config.text.empty()) return false;

    DrawText(config.text.c_str(), static_cast<int>(config.position.x), static_cast<int>(config.position.y),
        static_cast<int>(config.size.x),
             config.theme.color(WidgetConfig::Theme::ColorType::FONT));

    return true;
}

inline void _button_render(const uuid id, const WidgetConfig& config) {
    draw_if_kb_focus(id, [&]() {
        get().draw_widget(config.position, config.size * 1.05f, config.rotation,
                          color::teal, "TEXTURE");
    });

    if (get().hot_id == id) {
        if (get().active_id == id) {
            get().draw_widget(config.position, config.size, config.rotation,
                              color::red, "TEXTURE");
        } else {
            // Hovered
            get().draw_widget(config.position, config.size, config.rotation,
                              color::green, "TEXTURE");
        }
    } else {
        get().draw_widget(config.position, config.size, config.rotation,
                          color::blue, "TEXTURE");
    }

    get().draw_widget(config.position, config.size, config.rotation,
                      config.theme.color(), config.theme.texture);

    if (config.text.size() != 0) {
        WidgetConfig textConfig(config);
        // TODO detect if the button color is dark
        // and change the color to white automatically
        // textConfig.theme.fontColor =
        // getOppositeColor(config.theme.color());
        textConfig.position = config.position + vec2{config.size.x * 0.05f,
                                                     config.size.y * 0.25f};
        textConfig.size = vec2{config.size.y, config.size.y} * 0.75f;

        _text_impl(MK_UUID(id.ownerLayer, 0), textConfig);
    }
}

inline bool _button_pressed(const uuid id) {
    // check click
    if (has_kb_focus(id)) {
        if (get().pressed("Widget Press")) {
            return true;
        }
    }
    if (!get().lmouse_down && is_active_and_hot(id)) {
        get().kb_focus_id = id;
        return true;
    }
    return false;
}

bool _button_impl(const uuid id, WidgetConfig config) {
    // no state
    active_if_mouse_inside(id, Rectangle{config.position.x, config.position.y,
                                         config.size.x, config.size.y});
    try_to_grab_kb(id);
    _button_render(id, config);
    handle_tabbing(id);
    bool pressed = _button_pressed(id);
    return pressed;
}

bool _button_list_impl(const uuid id, WidgetConfig config,
                       const std::vector<WidgetConfig>& configs,
                       int* selectedIndex = nullptr, bool* hasFocus = nullptr) {
    auto state = get().widget_init<ButtonListState>(id);
    if (selectedIndex) state->selected.set(*selectedIndex);

    // TODO :HASFOCUS do we ever want to read the value
    // or do we want to reset focus each frame
    // if (hasFocus) state->hasFocus.set(*hasFocus);
    state->hasFocus.set(false);

    auto pressed = false;
    float spacing = config.size.y * 1.0f;
    // TODO support flipping text?
    // float sign = config.flipTextY ? 1.f : -1.f;
    float sign = 1.f;

    // Generate all the button ids
    std::vector<uuid> ids;
    for (size_t i = 0; i < configs.size(); i++) {
        ids.push_back(MK_UUID_LOOP(id.ownerLayer, id.hash, static_cast<int>(i)));
    }

    for (size_t i = 0; i < configs.size(); i++) {
        const uuid button_id = ids[i];
        WidgetConfig bwlconfig(config);
        bwlconfig.position =
            config.position + vec2{0.f, sign * spacing * (i + 1)};
        bwlconfig.text = configs[i].text;
        // bwlconfig.theme.backgroundColor = configs[i].theme.color();

        if (button_with_label(button_id, bwlconfig)) {
            state->selected = static_cast<int>(i);
            pressed = true;
        }
    }

    // NOTE: hasFocus is generally a readonly variable because
    // we dont insert it into state on button_list start see :HASFOCUS
    //
    // We use it here because we know that if its true, the caller
    // is trying to force this to have focus OR we had focus
    // last frame.
    //
    bool somethingFocused = hasFocus ? *hasFocus : false;
    // NOTE: this exists because we only to be able to move
    // up and down if we are messing with the button list directly
    // It would be annoying to be focused on the textfield (e.g.)
    // and pressing up accidentally would unfocus that and bring you to some
    // random button list somewhere**
    //
    // ** in this situation they have to be visible, so no worries about
    // arrow keying your way into a dropdown
    //
    for (size_t i = 0; i < configs.size(); i++) {
        somethingFocused |= has_kb_focus(ids[i]);
    }

    if (somethingFocused) {
        if (get().pressed("Widget Value Up")) {
            state->selected = state->selected - 1;
            if (state->selected < 0) state->selected = 0;
            get().kb_focus_id = ids[state->selected];
        }

        if (get().pressed("Widget Value Down")) {
            state->selected = state->selected + 1;
            if (state->selected > (int) configs.size() - 1)
                state->selected = static_cast<int>(configs.size() - 1);
            get().kb_focus_id = ids[state->selected];
        }
    }

    // NOTE: This has to be after value changes so that
    // hasFocus is handed back up to its parent correctly
    // this allows the dropdown to know when none of its children have focus
    // --
    // doing this above the keypress, allows a single frame
    // where neither the dropdown parent nor its children are in focus
    // and that causes the dropdown to close on selection change which isnt
    // what we want
    for (size_t i = 0; i < configs.size(); i++) {
        // if you got the kb focus somehow
        // (ie by clicking or tabbing)
        // then we will just make you selected
        if (has_kb_focus(ids[i])) state->selected = static_cast<int>(i);
        state->hasFocus = state->hasFocus | has_kb_focus(ids[i]);
    }

    if (state->hasFocus) get().kb_focus_id = ids[state->selected];
    if (hasFocus) *hasFocus = state->hasFocus;
    if (selectedIndex) *selectedIndex = state->selected;
    return pressed;
}

bool _dropdown_impl(const uuid id, WidgetConfig config,
                    const std::vector<WidgetConfig>& configs,
                    bool* dropdownState, int* selectedIndex) {
    auto state = get().widget_init<DropdownState>(id);
    if (dropdownState) state->on.set(*dropdownState);

    config.text = configs[selectedIndex ? *selectedIndex : 0].text;

    // Draw the main body of the dropdown
    auto pressed = button(id, config);

    // TODO when you tab to the dropdown
    // it would be nice if it opened
    // try_to_grab_kb(id);
    // if (has_kb_focus(id)) {
    // state->on = true;
    // }

    // TODO right now you can change values through tab or through
    // arrow keys, maybe we should only allow arrows
    // and then tab just switches to the next non dropdown widget

    // Text drawn after button so it shows up on top...
    //
    // TODO rotation is not really working correctly and so we have to
    // offset the V a little more than ^ in order to make it look nice
    auto offset =
        vec2{config.size.x - (state->on ? 1.f : 1.6f), config.size.y * -0.25f};
    text(MK_UUID(id.ownerLayer, id.hash),
         WidgetConfig(
             // TODO support getOppositeColor
             // {.fontColor = getOppositeColor(config.theme.color()),
             {
             .position = config.position + offset,
             .rotation = state->on ? 90.f : 270.f,
             .text = ">",
             .theme =
                  WidgetTheme(config.theme.color(WidgetTheme::ColorType::FONT),
                              config.theme.color())
              }));

    bool childrenHaveFocus = false;

    // NOTE: originally we only did this when the dropdown wasnt already
    // open but we should be safe to always do this
    // 1. doesnt toggle on, sets on directly
    // 2. open should have children focused anyway
    // 3. we dont eat the input, so it doesnt break the button_list value
    // up/down
    if (has_kb_focus(id)) {
        if (get().pressedWithoutEat("Widget Value Up") ||
            get().pressedWithoutEat("Widget Value Down")) {
            state->on = true;
            childrenHaveFocus = true;
        }
    }

    if (state->on) {
        if (button_list(MK_UUID(id.ownerLayer, id.hash), config, configs,
                        selectedIndex, &childrenHaveFocus)) {
            state->on = false;
            get().kb_focus_id = id;
        }
    }

    auto ret =
        *dropdownState != state->on.asT() || (pressed && state->on.asT());

    // NOTE: this has to happen after ret
    // because its not a real selection, just
    // a tab out
    if (!childrenHaveFocus && !has_kb_focus(id)) {
        state->on = false;
    }

    if (pressed) state->on = !state->on;
    if (dropdownState) *dropdownState = state->on;
    return ret;
}

//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////

bool text(const uuid id, const WidgetConfig& config) {
    return _text_impl(id, config);
}
bool button(const uuid id, WidgetConfig config) {
    return _button_impl(id, config);
}

bool button_with_label(const uuid id, WidgetConfig config) {
    auto pressed = button(id, config);
    if (config.text == "") {
        // apply offset so text is relative to button position
        config.child->position += config.position;
        text(MK_UUID(id.ownerLayer, 0), *config.child);
    }
    return pressed;
}

bool button_list(const uuid id, WidgetConfig config,
                 const std::vector<WidgetConfig>& configs, int* selectedIndex,
                 bool* hasFocus) {
    return _button_list_impl(id, config, configs, selectedIndex, hasFocus);
}

bool dropdown(const uuid id, WidgetConfig config,
              const std::vector<WidgetConfig>& configs, bool* dropdownState,
              int* selectedIndex) {
    return _dropdown_impl(id, config, configs, dropdownState, selectedIndex);
}

}  // namespace ui
