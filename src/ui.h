
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
    const std::string& content,
    // what color to use
    theme::Usage color_usage = theme::Usage::Font);

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
    const Widget& widget,
    // List of text configs that should show in the dropdown
    const std::vector<std::string>& options,
    // which item is selected
    int* selectedIndex = nullptr,
    //
    bool* hasFocus = nullptr);

bool dropdown(
    // returns true if dropdown value was changed
    const Widget& widget,
    // List of text configs that should show in the dropdown
    const std::vector<std::string>& options,
    // whether or not the dropdown is open
    bool* dropdownState,
    // which item is selected
    int* selectedIndex);

bool slider(
    // returns true if slider moved
    const Widget& widget,
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
        if (is_active(ROOT_ID) && get().lmouse_down) {
            get().active_id = id;
        }
    }
    return;
}

inline bool has_kb_focus(const uuid& id) { return (get().kb_focus_id == id); }

inline void try_to_grab_kb(const uuid id) {
    if (has_kb_focus(ROOT_ID)) {
        get().kb_focus_id = id;
    }
}

inline void draw_if_kb_focus(const uuid& id, std::function<void(void)> cb) {
    if (has_kb_focus(id)) cb();
}

inline void handle_tabbing(const uuid id) {
    // TODO How do we handle something that wants to use
    // Widget Value Down/Up to control the value?
    // Do we mark the widget type with "nextable"? (tab will always work but
    // not very discoverable
    if (has_kb_focus(id)) {
        if (get().pressed("Widget Next") /*||
            get().pressed("Widget Value Down")*/) {
            get().kb_focus_id = ROOT_ID;
            if (get().is_held_down("Widget Mod")) {
                get().kb_focus_id = get().last_processed;
            }
        }
        /*
        if (get().pressed("Widget Value Up")) {
            get().kb_focus_id = get().last_processed;
        }
        if (get().pressed("Widget Back")) {
            get().kb_focus_id = get().last_processed;
        }
        if (get().pressed("Widget Back")) {
            get().kb_focus_id = get().last_processed;
        }
        */
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
        get().draw_widget_old(
            position - offset,                                      //
            cs * (1.f + border_width),                              //
            0.f,                                                    //
            get().active_theme().from_usage(theme::Usage::Accent),  //
            "TEXTURE");
    });
}

inline void _button_render(Widget* widget_ptr) {
    Widget& widget = *widget_ptr;

    //
    auto lf = UIContext::LastFrame({.rect = widget.rect});
    get().write_last_frame(widget.id, lf);
    //

    _draw_focus_ring(widget);

    vec2 position = {
        widget.rect.x,
        widget.rect.y,
    };
    vec2 size = {
        widget.rect.width,
        widget.rect.height,
    };

    // if (is_hot(widget.id)) {
    // if (is_active(widget.id)) {
    // get().draw_widget_old(position, size, 0.f, color::red, "TEXTURE");
    // } else {
    // // Hovered
    // get().draw_widget_old(position, size, 0.f, color::green, "TEXTURE");
    // }
    // } else {
    // get().draw_widget_old(position, size, 0.f, color::black, "TEXTURE");
    // }

    UITheme theme = get().active_theme();
    Color color = is_active_and_hot(widget.id)
                      ? theme.from_usage(theme::Usage::Secondary)
                      : theme.from_usage(theme::Usage::Primary);

    get().draw_widget_old(position, size, 0.f, color, "TEXTURE");
}

inline bool _button_pressed(const uuid id) {
    // check click
    if (has_kb_focus(id)) {
        if (get().pressed("Widget Press")) {
            return true;
        }
    }
    if (get().lmouse_down && is_active_and_hot(id)) {
        get().kb_focus_id = id;
        return true;
    }
    return false;
}

bool _button_impl(const Widget& widget) {
    UIContext::LastFrame lf = get().get_last_frame(widget.id);
    bool pressed = false;
    if (lf.rect.has_value()) {
        active_if_mouse_inside(widget.id, lf.rect.value());

        try_to_grab_kb(widget.id);
        _button_render(widget.me);
        handle_tabbing(widget.id);
        pressed = _button_pressed(widget.id);
    }
    get().schedule_render_call(std::bind(_button_render, widget.me));
    return pressed;
}

bool _button_list_impl(const Widget& widget,
                       const std::vector<std::string>& options,
                       int* selectedIndex = nullptr, bool* hasFocus = nullptr) {
    auto state = get().widget_init<ButtonListState>(widget.id);
    if (selectedIndex) state->selected.set(*selectedIndex);

    // TODO :HASFOCUS do we ever want to read the value
    // or do we want to reset focus each frame
    // if (hasFocus) state->hasFocus.set(*hasFocus);
    state->hasFocus.set(false);
    auto pressed = false;

    get().push_parent(widget.me);
    std::vector<std::shared_ptr<Widget>> children;
    for (int i = 0; i < (int) options.size(); i++) {
        auto option = options[i];
        std::shared_ptr<Widget> button(
            new Widget(MK_UUID(widget.id.ownerLayer, widget.id),
                       {
                           .mode = Percent,
                           .value = 1.f,
                           .strictness = 1.f,
                       },
                       {
                           .mode = Percent,
                           .value = (1.f / options.size()),
                           .strictness = 0.9f,
                       }));

        if (button_with_label(*button, option)) {
            state->selected = static_cast<int>(i);
            pressed = true;
        }
        get().temp_widgets.push_back(button);
        children.push_back(button);
    }
    get().pop_parent();

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
    for (size_t i = 0; i < options.size(); i++) {
        somethingFocused |= has_kb_focus(children[i]->id);
    }

    if (somethingFocused) {
        if (get().pressed("Widget Value Up")) {
            state->selected = state->selected - 1;
            if (state->selected < 0) state->selected = 0;
            get().kb_focus_id = children[state->selected]->id;
        }

        if (get().pressed("Widget Value Down")) {
            state->selected = state->selected + 1;
            if (state->selected > (int) options.size() - 1)
                state->selected = static_cast<int>(options.size() - 1);
            get().kb_focus_id = children[state->selected]->id;
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
    for (size_t i = 0; i < options.size(); i++) {
        // if you got the kb focus somehow
        // (ie by clicking or tabbing)
        // then we will just make you selected
        if (has_kb_focus(children[i]->id))
            state->selected = static_cast<int>(i);
        state->hasFocus = state->hasFocus | has_kb_focus(children[i]->id);
    }

    if (state->hasFocus) get().kb_focus_id = children[state->selected]->id;
    if (hasFocus) *hasFocus = state->hasFocus;
    if (selectedIndex) *selectedIndex = state->selected;
    return pressed;
}

bool _dropdown_impl(const Widget widget,
                    const std::vector<std::string>& options,
                    bool* dropdownState, int* selectedIndex) {
    auto state = get().widget_init<DropdownState>(widget.id);
    if (dropdownState) state->on.set(*dropdownState);
    auto selected_option = options[selectedIndex ? *selectedIndex : 0];
    // Num options + 1 for the current selected
    int num_all = options.size() + 2;

    // TODO when you tab to the dropdown
    // it would be nice if it opened
    // try_to_grab_kb(id);
    // if (has_kb_focus(id)) {
    // state->on = true;
    // }

    bool return_value = false;
    bool pressed = false;

    get().push_parent(widget.me);
    {
        std::shared_ptr<Widget> selected_widget(
            new Widget(MK_UUID(widget.id.ownerLayer, widget.id),
                       {
                           .mode = Percent,
                           .value = 1.f,
                           .strictness = 1.f,
                       },
                       {
                           .mode = Percent,
                           .value = (1.f / num_all),
                           .strictness = 0.9f,
                       }));

        // TODO We should instead have get().request_temp() which returns
        // shared_ptr and ads to temp automatically
        get().temp_widgets.push_back(selected_widget);

        // Draw the main body of the dropdown
        pressed = button_with_label(*selected_widget, selected_option);

        // TODO right now you can change values through tab or through
        // arrow keys, maybe we should only allow arrows
        // and then tab just switches to the next non dropdown widget

        bool childrenHaveFocus = false;

        // NOTE: originally we only did this when the dropdown wasnt already
        // open but we should be safe to always do this
        // 1. doesnt toggle on, sets on directly
        // 2. open should have children focused anyway
        // 3. we dont eat the input, so it doesnt break the button_list value
        // up/down
        if (has_kb_focus(widget.id)) {
            if (get().pressedWithoutEat("Widget Value Up") ||
                get().pressedWithoutEat("Widget Value Down")) {
                state->on = true;
                childrenHaveFocus = true;
            }
        }

        if (state->on) {
            std::shared_ptr<Widget> button_list_widget(
                new Widget(MK_UUID(widget.id.ownerLayer, widget.id),
                           {
                               .mode = Percent,
                               .value = 1.f,
                               .strictness = 1.f,
                           },
                           {
                               .mode = Percent,
                               .value = (1.f / num_all),
                               .strictness = 0.9f,
                           }));

            if (button_list(*button_list_widget, options, selectedIndex,
                            &childrenHaveFocus)) {
                state->on = false;
                get().kb_focus_id = widget.id;
            }
            get().temp_widgets.push_back(button_list_widget);
        }

        return_value =
            *dropdownState != state->on.asT() || (pressed && state->on.asT());

        // NOTE: this has to happen after ret
        // because its not a real selection, just
        // a tab out
        if (!childrenHaveFocus && !has_kb_focus(widget.id)) {
            state->on = false;
        }
    }
    get().pop_parent();

    if (pressed) state->on = !state->on;
    if (dropdownState) *dropdownState = state->on;
    return return_value;
}

inline void _slider_render(Widget* widget_ptr, const bool vertical,
                           const float value) {
    Widget& widget = *widget_ptr;

    active_if_mouse_inside(widget.id, widget.rect);

    const auto cs = vec2{
        widget.rect.width,
        widget.rect.height,
    };
    const float maxScale = 0.8f;

    const float pos_offset =
        value * (vertical ? cs.y * maxScale : cs.x * maxScale);
    const auto pos = vec2{
        widget.rect.x,
        widget.rect.y,
    };

    _draw_focus_ring(widget);
    // slider rail
    Color rail = has_kb_focus(widget.id) ? get().active_theme().primary
                                         : get().active_theme().secondary;

    get().draw_widget_old(pos, cs, 0.f, rail, "TEXTURE");

    // slide
    vec2 offset = vertical ? vec2{0.f, pos_offset} : vec2{pos_offset, 0.f};
    vec2 size = vertical ? vec2{cs.x, cs.y / 5.f} : vec2{cs.x / 5.f, cs.y};

    // TODO chose a better color here or put one in theme
    const auto col = is_active_or_hot(widget.id)
                         ? color::red
                         : color::getOppositeColor(rail);
    get().draw_widget_old(pos + offset, size, 0.f, col, "TEXTURE");
}

void _slider_value_management(const Widget* widget, bool vertical, float* value,
                              float mnf, float mxf) {
    auto state = get().get_widget_state<SliderState>(widget->id);

    bool value_changed = false;
    if (has_kb_focus(widget->id)) {
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

    if (get().active_id == widget->id) {
        get().kb_focus_id = widget->id;
        float v;
        if (vertical) {
            v = (get().mouse.y - widget->rect.y) / widget->rect.height;
        } else {
            v = (get().mouse.x - widget->rect.x) / widget->rect.width;
        }
        if (v < mnf) v = mnf;
        if (v > mxf) v = mxf;
        if (v != *value) {
            state->value = v;
            (*value) = state->value;
            value_changed = true;
        }
    }
    state->value.changed_since = value_changed;
}

bool _slider_impl(const Widget& widget, bool vertical, float* value, float mnf,
                  float mxf) {
    // TODO be able to scroll this bar with the scroll wheel
    auto state = get().widget_init<SliderState>(widget.id);
    bool changed_previous_frame = state->value.changed_since;
    state->value.changed_since = false;
    if (value) state->value.set(*value);

    // dont mind if i do
    try_to_grab_kb(widget.id);
    get().schedule_render_call(
        std::bind(_slider_render, widget.me, vertical, state->value.asT()));
    handle_tabbing(widget.id);
    get().schedule_render_call(std::bind(_slider_value_management, widget.me,
                                         vertical, value, mnf, mxf));
    return changed_previous_frame;
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

void init_widget(const Widget& widget, const char* func,
                 bool uses_state = false) {
    // TODO today because of the layer check we cant use this
    // which means no good error reports on this issue
    // you will just segfault
    //
    // Basically the issue is that because the MK_UUID is in widget constructor
    // the hash is the same for two ui elements.
    //
    // This will only happen if you have two Widgets that you construct on the
    // same layer. This is because we use layer id (static atomic inc) as a hash
    // key In order for us to get the "default" state id for a layer, we need
    // either the layer id or an example widget

    // if (uses_state && widget.id ==
    // get().default_state_id) { std::cout << "You need to initialize the id on
    // this widget, because it " "uses global state." " In your widget
    // contructor pass 'MK_UUID(id, ROOT_ID)' " "as the first parameter."
    // << std::endl;
    // }
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

bool text(const Widget& widget, const std::string& content,
          theme::Usage color_usage) {
    init_widget(widget, __FUNCTION__);
    // No need to render if text is empty
    if (content.empty()) return false;
    get().schedule_draw_text(widget.me, content, color_usage);
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
            new Widget(MK_UUID(widget.id.ownerLayer, widget.id),
                       {.mode = Percent, .value = 1.f, .strictness = 1.0f},
                       {.mode = Percent, .value = 1.f, .strictness = 1.0f}));
        get().temp_widgets.push_back(internal_button);

        pressed = button(*internal_button);
        std::shared_ptr<Widget> internal_text(
            new Widget({.mode = Percent, .value = 1.f, .strictness = 1.0f},
                       {.mode = Percent, .value = 1.f, .strictness = 1.0f}));
        get().temp_widgets.push_back(internal_text);
        text(*internal_text, content, theme::Usage::DarkFont);
    }
    get().pop_parent();
    return pressed;
}

bool button_list(const Widget& widget, const std::vector<std::string>& options,
                 int* selectedIndex, bool* hasFocus) {
    init_widget(widget, __FUNCTION__, true);
    return _button_list_impl(widget, options, selectedIndex, hasFocus);
}

bool dropdown(const Widget& widget, const std::vector<std::string>& options,
              bool* dropdownState, int* selectedIndex) {
    init_widget(widget, __FUNCTION__, true);
    return _dropdown_impl(widget, options, dropdownState, selectedIndex);
}

bool slider(const Widget& widget, bool vertical, float* value, float mnf,
            float mxf) {
    init_widget(widget, __FUNCTION__, true);
    return _slider_impl(widget, vertical, value, mnf, mxf);
}

}  // namespace ui
