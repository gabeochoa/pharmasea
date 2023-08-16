
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

///

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
