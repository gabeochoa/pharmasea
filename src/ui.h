
#pragma once

#include "external_include.h"
//
#include "ui_color.h"
#include "ui_context.h"
#include "ui_state.h"
#include "ui_widget.h"
#include "ui_widget_config.h"
#include "uuid.h"

namespace ui {

// TODO add more info about begin()/end()

bool text(
    // returns true if content isnt empty
    const Widget& widget,
    // the text to render
    const std::string& content);

bool button(
    // returns true if the button was clicked else false
    const Widget& widget);

bool button_with_label(
    // returns true if the button was clicked else false
    const Widget& widget,
    // button label
    const std::string& content);

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

bool slider(
    // returns true if slider moved
    const uuid id, WidgetConfig config,
    // Current value of the slider
    float* value,
    // min value
    float mnf,
    // max value
    float mxf);

///////// ////// ////// ////// ////// ////// ////// //////
///////// ////// ////// ////// ////// ////// ////// //////
///////// ////// ////// ////// ////// ////// ////// //////
///////// ////// ////// ////// ////// ////// ////// //////
///////// ////// ////// ////// ////// ////// ////// //////
///////// ////// ////// ////// ////// ////// ////// //////
///////// ////// ////// ////// ////// ////// ////// //////
///////// ////// ////// ////// ////// ////// ////// //////
///////// ////// ////// ////// ////// ////// ////// //////
///////// ////// ////// ////// ////// ////// ////// //////
///////// ////// ////// ////// ////// ////// ////// //////
///////// ////// ////// ////// ////// ////// ////// //////
///
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
        if (get().pressed("Widget Back")) {
            get().kb_focus_id = get().last_processed;
        }
    }
    // before any returns
    get().last_processed = id;
}

void _draw_focus_ring(const Widget& widget) {
    draw_if_kb_focus(widget.id, [&]() {
        const auto position = vec2{
            widget.rect.x,
            widget.rect.y,
        };
        const auto cs = vec2{widget.rect.width, widget.rect.height};
        const auto border_width = 0.05f;
        const auto offset =
            vec2{cs.x * (border_width / 2), cs.y * (border_width / 2)};
        get().draw_widget_old(position - offset,          //
                              cs * (1.f + border_width),  //
                              0.f,                        //
                              color::white,               //
                              "TEXTURE");
    });
}

inline void _button_render(Widget* widget_ptr) {
    Widget& widget = *widget_ptr;
    _draw_focus_ring(widget);

    vec2 position = {
        widget.rect.x,
        widget.rect.y,
    };
    vec2 size = {
        widget.rect.width,
        widget.rect.height,
    };
    std::cout << "button render " << position << "  " << size << std::endl;

    if (get().hot_id == widget.id) {
        if (get().active_id == widget.id) {
            get().draw_widget_old(position, size, 0.f, color::red, "TEXTURE");
        } else {
            // Hovered
            get().draw_widget_old(position, size, 0.f, color::green, "TEXTURE");
        }
    } else {
        get().draw_widget_old(position, size, 0.f, color::black, "TEXTURE");
    }

    UITheme theme = get().active_theme();
    Color color = has_kb_focus(widget.id)
                      ? theme.from_usage(theme::Usage::Primary)
                      : theme.from_usage(theme::Usage::Secondary);

    get().draw_widget_old(position, size, 0.f, color, "TEXTURE");

    // if (config.text.size() != 0) {
    // WidgetConfig textConfig(config);
    // // TODO detect if the button color is dark
    // // and change the color to white automatically
    // textConfig.theme.fontColor = color::getOppositeColor(color);
    // textConfig.position =
    // config.position + vec2{config.size.x * 0.05f, config.size.y * 0.1f};
    // textConfig.size = vec2{config.size.y, config.size.y};
    //
    // text_old(MK_UUID(id.ownerLayer, 0), textConfig);
    // }
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

bool _button_impl(const Widget& widget) {
    // no state
    active_if_mouse_inside(widget.id, widget.rect);
    try_to_grab_kb(widget.id);
    std::cout << "scheduling old render for " << &widget << std::endl;
    get().schedule_render_call(std::bind(_button_render, widget.me));
    handle_tabbing(widget.id);
    bool pressed = _button_pressed(widget.id);
    return pressed;
}

/*
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
        ids.push_back(
            MK_UUID_LOOP(id.ownerLayer, id.hash, static_cast<int>(i)));
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
    text_old(MK_UUID(id.ownerLayer, id.hash),
             WidgetConfig(
                 // TODO support getOppositeColor
                 // {.fontColor = getOppositeColor(config.theme.color()),
                 {.position = config.position + offset,
                  .rotation = state->on ? 90.f : 270.f,
                  .text = ">",
                  .theme = WidgetTheme(
                      config.theme.color(WidgetTheme::ColorType::FONT),
                      config.theme.color())}));

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

inline void _slider_render(const uuid id, const WidgetConfig& config,
                           const float value) {
    const auto cs = config.size;
    const float maxScale = 0.8f;
    const float pos_offset =
        value * (config.vertical ? cs.y * maxScale : cs.x * maxScale);
    const auto pos = config.position;
    const auto tex = config.theme.texture;

    _draw_focus_ring(id, config);
    // slider rail
    Color rail = has_kb_focus(id)
                     ? color::getOppositeColor(config.theme.color())
                     : config.theme.color();
    get().draw_widget_old(pos, cs, config.rotation, rail, tex);

    // slide
    vec2 offset =
        config.vertical ? vec2{0.f, pos_offset} : vec2{pos_offset, 0.f};
    vec2 size =
        config.vertical ? vec2{cs.x, cs.y / 5.f} : vec2{cs.x / 5.f, cs.y};

    // TODO chose a better color here or put one in theme
    const auto col =
        is_active_or_hot(id) ? color::red : color::getOppositeColor(rail);
    get().draw_widget_old(config.position + offset, size, config.rotation, col,
                          tex);
}

bool _slider_impl(const uuid id, WidgetConfig config, float* value, float mnf,
                  float mxf) {
    // TODO be able to scroll this bar with the scroll wheel
    auto state = get().widget_init<SliderState>(id);
    if (value) state->value.set(*value);

    active_if_mouse_inside(id, Rectangle{config.position.x, config.position.y,
                                         config.size.x, config.size.y});
    // dont mind if i do
    try_to_grab_kb(id);
    _slider_render(id, config, state->value.asT());
    handle_tabbing(id);

    bool value_changed = false;
    if (has_kb_focus(id)) {
        if (get().pressed("Value Up")) {
            state->value = state->value + 0.005;
            if (state->value > mxf) state->value = mxf;

            (*value) = state->value;
            value_changed = true;
        }
        if (get().pressed("Value Down")) {
            state->value = state->value - 0.005;
            if (state->value < mnf) state->value = mnf;
            (*value) = state->value;
            value_changed = true;
        }
    }

    if (get().active_id == id) {
        get().kb_focus_id = id;
        float v;
        if (config.vertical) {
            v = (config.position.y - get().mouse.y) / config.size.y;
        } else {
            v = (get().mouse.x - config.position.x) / config.size.x;
        }
        if (v < mnf) v = mnf;
        if (v > mxf) v = mxf;
        if (v != *value) {
            state->value = v;
            (*value) = state->value;
            value_changed = true;
        }
    }
    return value_changed;
}
*/

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

void init_widget(const Widget& widget, const char* func){
    Widget::set_element(widget, func);
    get().add_child(widget.me);
}

bool padding(const Widget& widget) {
    init_widget(widget, __FUNCTION__);
    return true;
}

bool div(const Widget& widget) {
    init_widget(widget, __FUNCTION__);
    return true;
}

bool text(const Widget& widget, const std::string& content) {
    init_widget(widget, __FUNCTION__);
    // No need to render if text is empty
    if (content.empty()) return false;
    get().schedule_draw_text(widget.me, content);
    return true;
}

bool button(const Widget& widget) {
    init_widget(widget, __FUNCTION__);
    return _button_impl(widget);
}

bool button_with_label(const Widget& widget, const std::string& content) {
    init_widget(widget, __FUNCTION__);
    bool pressed = false;
    get().push_parent(widget.me);
    {
        std::shared_ptr<Widget> internal_button(
            new Widget({.mode = Percent, .value = 1.f, .strictness = 1.0f},
                       {.mode = Percent, .value = 1.f, .strictness = 1.0f}));
        get().temp_widgets.push_back(internal_button);

        pressed = button(*internal_button);
        std::shared_ptr<Widget> internal_text(
            new Widget({.mode = Percent, .value = 1.f, .strictness = 1.0f},
                       {.mode = Percent, .value = 1.f, .strictness = 1.0f}));
        get().temp_widgets.push_back(internal_text);
        text(*internal_text, content);
    }
    get().pop_parent();
    return pressed;
}

/*
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

bool slider(const uuid id, WidgetConfig config, float* value, float mnf,
            float mxf) {
    return _slider_impl(id, config, value, mnf, mxf);
}
*/
}  // namespace ui
