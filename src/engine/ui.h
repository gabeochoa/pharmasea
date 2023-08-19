
#pragma once

#include "raylib.h"
//
#include "ui_autolayout.h"
#include "ui_color.h"
#include "ui_components.h"
#include "ui_context.h"
#include "ui_state.h"
#include "ui_theme.h"
#include "ui_widget.h"

//
#include "texture_library.h"
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
    const std::string& content = "",
    // if true, hides the button background
    const bool no_background = false);

bool image_button(
    // returns true if the button was clicked else false
    const Widget& widget,
    // button label if needed
    const std::string& texture_name = "");

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

enum struct TextfieldValidationDecisionFlag {
    // Nothing
    None = 0,
    // Stop any new forward input (ie max length)
    StopNewInput = 1 << 0,
    // Show checkmark / green text
    Valid = 1 << 1,
    // Show x / red text
    Invalid = 1 << 2,
};  // namespace ui

inline bool operator!(TextfieldValidationDecisionFlag f) {
    return f == TextfieldValidationDecisionFlag::None;
}

inline bool validation_test(TextfieldValidationDecisionFlag flag,
                            TextfieldValidationDecisionFlag mask) {
    return !!(flag & mask);
}

typedef std::function<TextfieldValidationDecisionFlag(const std::string&)>
    TextFieldValidationFn;

// TODO add ability to have a cursor and move the cursor with arrow keys
bool textfield(
    // returns true if text changed
    const Widget& widget,
    // the string value being edited
    std::string& content,
    // validation function that returns a flag describing what the text
    // input should do
    TextFieldValidationFn validation = {});

bool checkbox(
    // Returns true if the checkbox changed
    const Widget& widget,
    // whether or not the checkbox is X'd
    bool* cbState = nullptr,
    // optional label to replace X and ``
    std::string* label = nullptr);

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
        if (is_active(ROOT_ID) && get().mouse_info.leftDown) {
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
        if (
            //
            get().pressed(InputName::WidgetNext) ||
            get().pressed(InputName::ValueDown)
            // TODO add support for holding down tab
            // get().is_held_down_debounced(InputName::WidgetNext) ||
            // get().is_held_down_debounced(InputName::ValueDown)
        ) {
            get().kb_focus_id = ROOT_ID;
            if (get().is_held_down(InputName::WidgetMod)) {
                get().kb_focus_id = get().last_processed;
            }
        }
        if (get().pressed(InputName::ValueUp)) {
            get().kb_focus_id = get().last_processed;
        }
        if (get().pressed(InputName::WidgetBack)) {
            get().kb_focus_id = get().last_processed;
        }
    }
    // before any returns
    get().last_processed = id;
}

inline void _draw_focus_ring(const Widget& widget,
                             theme::Usage usage = theme::Usage::Accent) {
    draw_if_kb_focus(widget.id, [&]() {
        const auto position = vec2{
            widget.rect.x,
            widget.rect.y,
        };
        const auto cs = vec2{widget.rect.width, widget.rect.height};
        const auto border_width = 0.05f;
        const auto offset =
            vec2{cs.x * (border_width / 2), cs.y * (border_width / 2)};
        const auto new_pos = position - offset;
        const auto new_cs = cs * (1.f + border_width);
        get().draw_widget_rect(
            {
                new_pos.x,
                new_pos.y,
                new_cs.x,
                new_cs.y,
            },
            usage);
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

inline UIContext::LastFrame init_widget(const Widget& widget,
                                        const char* func) {
    // TODO today because of the layer check we cant use this
    // which means no good error reports on this issue
    // you will just segfault
    //
    // Basically the issue is that because the MK_UUID is in widget
    // constructor the hash is the same for two ui elements.
    //
    // This will only happen if you have two Widgets that you construct on
    // the same layer. This is because we use layer id (static atomic inc)
    // as a hash key In order for us to get the "default" state id for a
    // layer, we need either the layer id or an example widget

    // if (uses_state && widget.id ==
    // get().default_state_id) { log_warn("You need to initialize the id on
    // this widget, because it " "uses global state." " In your widget
    // contructor pass 'MK_UUID(id, ROOT_ID)' " "as the first parameter.");
    // }
    Widget::set_element(widget, func);
    get().add_child(widget.me);

    const auto _write_lf = [](Widget* widget_ptr) {
        const Widget& widget = *widget_ptr;
        auto lf = UIContext::LastFrame({.rect = widget.rect});
        get().write_last_frame(widget.id, lf);
    };

    UIContext::LastFrame lf = get().get_last_frame(widget.id);
    get().schedule_render_call(std::bind(_write_lf, widget.me));
    return lf;
}

inline bool padding(const Widget& widget) {
    init_widget(widget, __FUNCTION__);
    return true;
}

inline bool div(const Widget& widget) {
    init_widget(widget, __FUNCTION__);
    return true;
}

inline bool text(const Widget& widget, const std::string& content,
                 theme::Usage color_usage) {
    init_widget(widget, __FUNCTION__);
    // No need to render if text is empty
    if (content.empty()) return false;
    get().schedule_draw_text(widget.me, content, color_usage);
    return true;
}

inline bool button(const Widget& widget, const std::string& content,
                   bool no_background) {
    const auto _button_pressed = [](const uuid id) {
        // check click
        if (has_kb_focus(id) && get().pressed(InputName::WidgetPress)) {
            return true;
        }
        if (!get().mouse_info.leftDown && is_active_and_hot(id)) {
            get().kb_focus_id = id;
            return true;
        }
        return false;
    };

    UIContext::LastFrame lf = init_widget(widget, __FUNCTION__);
    if (!lf.rect.has_value()) return false;

    {
        widget.me->rect = lf.rect.value();

        active_if_mouse_inside(widget.id, lf.rect.value());
        try_to_grab_kb(widget.id);

        _draw_focus_ring(widget);
        if (!no_background) {
            get().draw_widget_rect(widget.rect, is_active_and_hot(widget.id)
                                                    ? (theme::Usage::Secondary)
                                                    : (theme::Usage::Primary));
        }
        get()._draw_text(widget.rect, content, theme::Usage::Font);

        handle_tabbing(widget.id);
    }
    return _button_pressed(widget.id);
}

}  // namespace ui
