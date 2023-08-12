
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

/*
inline bool button_list(const Widget& widget,
                        const std::vector<std::string>& options,
                        int* selectedIndex, bool* hasFocus) {
    init_widget(widget, __FUNCTION__);
    auto state = get().widget_init<ButtonListState>(widget.id);
    if (selectedIndex) state->selected.set(*selectedIndex);

    // TODO this assert isn't working
    // M_ASSERT(widget.growflags & GrowFlags::Column,
    // "button lists must have growflags Column");

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
            new Widget(MK_UUID_LOOP(widget.id.ownerLayer, widget.id, i),
                       {
                           .mode = Percent,
                           .value = 1.f,
                           .strictness = 1.f,
                       },
                       {
                           .mode = Percent,
                           .value = 1.f / options.size(),
                           .strictness = 1.f,
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
        if (get().pressed(InputName::ValueUp)) {
            state->selected = state->selected - 1;
            if (state->selected < 0) state->selected = 0;
            get().kb_focus_id = children[state->selected]->id;
        }

        if (get().pressed(InputName::ValueDown)) {
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
        state->hasFocus = state->hasFocus || has_kb_focus(children[i]->id);
    }

    if (state->hasFocus) get().kb_focus_id = children[state->selected]->id;
    if (hasFocus) *hasFocus = state->hasFocus;
    if (selectedIndex) *selectedIndex = state->selected;
    return pressed;
}
*/

/*
inline bool textfield(const Widget& widget, std::string& content,
                      TextFieldValidationFn validation) {
    auto _textfield_render = [](Widget* widget_ptr,
                                TextFieldValidationFn validation) {
        auto state = get().get_widget_state<TextfieldState>(widget_ptr->id);
        const Widget& widget = *widget_ptr;

        TextfieldValidationDecisionFlag validationFlag =
            validation ? validation(state->buffer.asT())
                       : TextfieldValidationDecisionFlag::None;

        bool is_invalid = validation_test(
            validationFlag, TextfieldValidationDecisionFlag::Invalid);
        auto focus_color =
            is_invalid ? theme::Usage::Error : theme::Usage::Accent;
        _draw_focus_ring(widget, focus_color);
        // Background
        get().draw_widget(widget, is_active_and_hot(widget.id)
                                      ? theme::Usage::Secondary
                                      : theme::Usage::Primary);

        bool shouldWriteCursor = has_kb_focus(widget.id) && state->showCursor;
        std::string focusStr = shouldWriteCursor ? "_" : "";
        std::string focused_content =
            fmt::format("{}{}", state->buffer.asT(), focusStr);

        auto text_color = is_invalid ? theme::Usage::Error : theme::Usage::Font;
        get()._draw_text(widget.rect, focused_content, text_color);
    };

    const auto _textfield_value_management =
        [](const Widget* widget, TextFieldValidationFn validation) {
            auto state = get().get_widget_state<TextfieldState>(widget->id);

            auto validationFlag = validation
                                      ? validation(state->buffer.asT())
                                      : TextfieldValidationDecisionFlag::None;

            state->cursorBlinkTime = state->cursorBlinkTime + 1;
            if (state->cursorBlinkTime > 60) {
                state->cursorBlinkTime = 0;
                state->showCursor = !state->showCursor;
            }

            bool changed = false;

            if (has_kb_focus(widget->id)) {
                if (get().keychar != int()) {
                    if (validation_test(
                            validationFlag,
                            TextfieldValidationDecisionFlag::StopNewInput)) {
                    } else {
                        state->buffer.asT().append(
                            std::string(1, (char) get().keychar));
                        changed = true;
                    }
                }
                if (get().pressed(InputName::WidgetBackspace) ||
                    get().is_held_down_debounced(InputName::WidgetBackspace)) {
                    if (state->buffer.asT().size() > 0) {
                        state->buffer.asT().pop_back();
                    }
                    changed = true;
                }

                if (get().is_held_down(InputName::WidgetCtrl)) {
                    if (get().pressed(InputName::WidgetPaste)) {
                        auto clipboard = ext::get_clipboard_text();

                        // make a copy so we can text validation
                        std::string post_paste(state->buffer.asT());
                        post_paste.append(clipboard);

                        auto vflag =
                            validation ? validation(post_paste)
                                       : TextfieldValidationDecisionFlag::None;
                        // TODO we should paste the amount that fits. but we
                        // dont know what the max length actually is, so we
                        // would need to remove one letter at a time until we
                        // hit something valid.

                        // TODO we probably should give some kind of visual
                        // error to the user that you cant paste right now
                        bool should_append = !validation_test(
                            vflag,
                            TextfieldValidationDecisionFlag::StopNewInput);

                        // commit the copy-paste
                        if (should_append) state->buffer.asT() = post_paste;
                    }
                    changed = true;
                }
            }

            if (get().mouse_info.leftDown && is_active_and_hot(widget->id)) {
                get().kb_focus_id = widget->id;
            }
            state->buffer.changed_since = changed;
        };

    UIContext::LastFrame lf = init_widget(widget, __FUNCTION__);
    auto state = get().widget_init<TextfieldState>(widget.id);
    bool changed_previous_frame = state->buffer.changed_since;

    if (!content.empty()) state->buffer = content;
    state->buffer.changed_since = false;

    if (!lf.rect.has_value()) return changed_previous_frame;

    widget.me->rect = lf.rect.value();
    try_to_grab_kb(widget.id);
    active_if_mouse_inside(widget.id, widget.rect);
    _textfield_render(widget.me, validation);
    _textfield_value_management(widget.me, validation);
    handle_tabbing(widget.id);

    content = state->buffer;
    return changed_previous_frame;
}
*/

}  // namespace ui
