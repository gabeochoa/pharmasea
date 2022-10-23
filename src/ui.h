
#pragma once

#include "external_include.h"
//
#include "ui_color.h"
#include "ui_context.h"
#include "ui_state.h"
#include "ui_theme.h"
#include "ui_widget.h"
#include "uuid.h"

namespace ui {

// NOTE: Because we use mutate widget's data during layout
//       do not pass the same widget twice. (even if they are the same size).
//       You can only do this if they are .strictness 1.0 or .ignore_size = true
//
//  Bad:
//          Widget b = ...;
//          button(b);
//          button(b);
//
//  Good:
//          Widget a = ...;
//          Widget b = ...;
//          button(a);
//          button(b);
//

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
    const Widget& widget,
    // button label if needed
    const std::string& content = "");

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

bool textfield(
    // returns true if text changed
    const Widget& widget, std::string& content);

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
        if (get().pressed("Widget Next") || get().pressed("Value Down")) {
            get().kb_focus_id = ROOT_ID;
            if (get().is_held_down("Widget Mod")) {
                get().kb_focus_id = get().last_processed;
            }
        }
        if (get().pressed("Value Up")) {
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
        get().draw_widget_old(
            position - offset,                                      //
            cs * (1.f + border_width),                              //
            0.f,                                                    //
            get().active_theme().from_usage(theme::Usage::Accent),  //
            "TEXTURE");
    });
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

void init_widget(const Widget& widget, const char* func) {
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

bool button(const Widget& widget, const std::string& content) {
    const auto _write_lf = [](Widget* widget_ptr) {
        Widget& widget = *widget_ptr;
        auto lf = UIContext::LastFrame({.rect = widget.rect});
        get().write_last_frame(widget.id, lf);
    };

    const auto _button_render = [](Widget* widget_ptr) {
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
    };

    const auto _button_pressed = [](const uuid id) {
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
    };

    init_widget(widget, __FUNCTION__);
    UIContext::LastFrame lf = get().get_last_frame(widget.id);
    bool pressed = false;
    if (lf.rect.has_value()) {
        widget.me->rect = lf.rect.value();
        active_if_mouse_inside(widget.id, lf.rect.value());
        try_to_grab_kb(widget.id);
        _button_render(widget.me);
        get()._draw_text(widget.rect, content, theme::Usage::Font);
        handle_tabbing(widget.id);
        pressed = _button_pressed(widget.id);
    }
    get().schedule_render_call(std::bind(_write_lf, widget.me));
    return pressed;
}

bool button_list(const Widget& widget, const std::vector<std::string>& options,
                 int* selectedIndex, bool* hasFocus) {
    init_widget(widget, __FUNCTION__);
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
        std::shared_ptr<Widget> button_w = get().make_temp_widget(
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

        if (button(*button_w, option)) {
            state->selected = static_cast<int>(i);
            pressed = true;
        }
        children.push_back(button_w);
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
        if (get().pressed("Value Up")) {
            state->selected = state->selected - 1;
            if (state->selected < 0) state->selected = 0;
            get().kb_focus_id = children[state->selected]->id;
        }

        if (get().pressed("Value Down")) {
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

bool dropdown(const Widget& widget, const std::vector<std::string>& options,
              bool* dropdownState, int* selectedIndex) {
    init_widget(widget, __FUNCTION__);
    auto state = get().widget_init<DropdownState>(widget.id);
    if (dropdownState) state->on.set(*dropdownState);
    auto selected_option = options[selectedIndex ? *selectedIndex : 0];
    // Num options + 1 for the current selected
    size_t num_all = options.size() + 2;

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
        std::shared_ptr<Widget> selected_widget = get().make_temp_widget(
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
                       })

        );

        // Draw the main body of the dropdown
        pressed = button(*selected_widget, selected_option);

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
            if (get().pressedWithoutEat("Value Up") ||
                get().pressedWithoutEat("Value Down")) {
                state->on = true;
                childrenHaveFocus = true;
            }
        }

        if (state->on) {
            std::shared_ptr<Widget> button_list_widget = get().make_temp_widget(
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

bool slider(const Widget& widget, bool vertical, float* value, float mnf,
            float mxf) {
    const auto _slider_render = [](Widget* widget_ptr, const bool vertical,
                                   const float value) {
        Widget& widget = *widget_ptr;

        //
        auto lf = UIContext::LastFrame({.rect = widget.rect});
        get().write_last_frame(widget.id, lf);
        //

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
        Color rail = get().active_theme().primary;
        get().draw_widget_old(pos, cs, 0.f, rail, "TEXTURE");

        // slide
        vec2 offset = vertical ? vec2{0.f, pos_offset} : vec2{pos_offset, 0.f};
        vec2 size = vertical ? vec2{cs.x, cs.y / 5.f} : vec2{cs.x / 5.f, cs.y};

        const auto col = get().active_theme().accent;
        get().draw_widget_old(pos + offset, size, 0.f, col, "TEXTURE");
    };

    const auto _slider_value_management = [](const Widget* widget,
                                             bool vertical, float* value,
                                             float mnf, float mxf) {
        auto state = get().get_widget_state<SliderState>(widget->id);

        bool value_changed = false;
        if (has_kb_focus(widget->id)) {
            if (get().is_held_down("Value Right")) {
                state->value = state->value + 0.005f;
                if (state->value > mxf) state->value = mxf;

                (*value) = state->value;
                value_changed = true;
            }
            if (get().is_held_down("Value Left")) {
                state->value = state->value - 0.005f;
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
    };

    init_widget(widget, __FUNCTION__);
    UIContext::LastFrame lf = get().get_last_frame(widget.id);
    // TODO be able to scroll this bar with the scroll wheel
    auto state = get().widget_init<SliderState>(widget.id);
    bool changed_previous_frame = state->value.changed_since;
    state->value.changed_since = false;
    if (value) state->value.set(*value);

    if (lf.rect.has_value()) {
        widget.me->rect = lf.rect.value();
        try_to_grab_kb(widget.id);
        _slider_render(widget.me, vertical, state->value.asT());
        handle_tabbing(widget.id);
    }

    get().schedule_render_call(
        std::bind(_slider_render, widget.me, vertical, state->value.asT()));
    get().schedule_render_call(std::bind(_slider_value_management, widget.me,
                                         vertical, value, mnf, mxf));
    return changed_previous_frame;
}

bool textfield(const Widget& widget, std::string& content) {
    init_widget(widget, __FUNCTION__);
    UIContext::LastFrame lf = get().get_last_frame(widget.id);
    auto state = get().widget_init<TextfieldState>(widget.id);
    bool changed_previous_frame = state->buffer.changed_since;
    state->buffer.changed_since = false;

    auto _textfield_render = [](Widget* widget_ptr) {
        auto state = get().get_widget_state<TextfieldState>(widget_ptr->id);
        Widget& widget = *widget_ptr;

        //
        auto lf = UIContext::LastFrame({.rect = widget.rect});
        get().write_last_frame(widget.id, lf);
        //

        active_if_mouse_inside(widget.id, widget.rect);
        _draw_focus_ring(widget);

        const auto pos = vec2{
            widget.rect.x,
            widget.rect.y,
        };

        const auto cs = vec2{
            widget.rect.width,
            widget.rect.height,
        };

        // background
        UITheme theme = get().active_theme();
        Color color = is_active_and_hot(widget.id)
                          ? theme.from_usage(theme::Usage::Secondary)
                          : theme.from_usage(theme::Usage::Primary);

        // Background
        get().draw_widget_old(pos, cs, 0.f, color, "TEXTURE");

        bool shouldWriteCursor = has_kb_focus(widget.id) && state->showCursor;
        std::string focusStr = shouldWriteCursor ? "_" : "";
        std::string focused_content =
            fmt::format("{}{}", state->buffer.asT(), focusStr);

        // TODO add support for string
        get()._draw_text(widget.rect, focused_content,
                         theme::Usage::Font);
    };

    const auto _textfield_value_management = [](const Widget* widget) {
        auto state = get().get_widget_state<TextfieldState>(widget->id);

        state->cursorBlinkTime = state->cursorBlinkTime + 1;
        if (state->cursorBlinkTime > 60) {
            state->cursorBlinkTime = 0;
            state->showCursor = !state->showCursor;
        }
        bool changed = false;

        if (has_kb_focus(widget->id)) {
            if (get().keychar != int()) {
                state->buffer.asT().append(std::string(1, (char) get().keychar));
                changed = true;
            }
            if (get().pressed("Widget Backspace")) {
                if (state->buffer.asT().size() > 0) {
                    state->buffer.asT().pop_back();
                }
                changed = true;
            }
        }

        if (get().lmouse_down && is_active_and_hot(widget->id)) {
            get().kb_focus_id = widget->id;
        }
        state->buffer.changed_since = changed;
    };

    if (lf.rect.has_value()) {
        widget.me->rect = lf.rect.value();
        try_to_grab_kb(widget.id);
        _textfield_render(widget.me);
        _textfield_value_management(widget.me);
        handle_tabbing(widget.id);
    }

    get().schedule_render_call(std::bind(_textfield_render, widget.me));
    content = state->buffer;
    return changed_previous_frame;
}

}  // namespace ui
