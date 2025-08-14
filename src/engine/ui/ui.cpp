

#include "ui.h"

#include "../../preload.h"
#include "../../vec_util.h"
#include "../assert.h"
#include "../event.h"
#include "../font_sizer.h"
#include "../font_util.h"
#include "../gamepad_axis_with_dir.h"
#include "../keymap.h"
#include "../log.h"
#include "../statemanager.h"
#include "../texture_library.h"
#include "../type_name.h"
#include "../uuid.h"
#include "autolayout.h"
#include "callback_registry.h"
#include "color.h"
#include "element_result.h"
#include "focus.h"
#include "raylib.h"
#include "sound.h"
#include "state.h"
#include "theme.h"
#include "ui_internal.h"
#include "widget.h"

namespace ui {

std::shared_ptr<ui::UIContext> context;

namespace focus {
int focus_id = ROOT_ID;
int last_processed = ROOT_ID;

int hot_id = ROOT_ID;
int active_id = ROOT_ID;
MouseInfo mouse_info;

std::set<int> ids;
}  // namespace focus

std::atomic_int UICONTEXT_ID = 0;
struct UIContext;
std::shared_ptr<UIContext> _uicontext;
UIContext* globalContext;

CallbackRegistry callback_registry;

void end() {
    callback_registry.execute_callbacks();
    focus::end();

    context->cleanup();
}

ElementResult hoverable(const Widget& widget) {
    focus::active_if_mouse_inside(widget);

    if (focus::is_hot(widget.id)) {
        return true;
    }
    return false;
}

ElementResult div(const Widget& widget, Color c, bool rounded) {
    Rectangle rect = widget.get_rect();
    internal::draw_rect_color(rect, widget.z_index, c, rounded);
    return true;
}

ElementResult div(const Widget& widget, theme::Usage theme, bool rounded) {
    return div(widget, UI_THEME.from_usage(theme), rounded);
}

ElementResult window(const Widget& widget) {
    internal::draw_rect(widget.rect, widget.z_index, ui::theme::Secondary);
    return ElementResult{true, widget.z_index - 1};
}

// TODO merge with text()
ElementResult colored_text(const Widget& widget,
                           const TranslatableString& content, Color c) {
    Rectangle rect = widget.get_rect();

    // No need to render if text is empty
    if (content.empty()) return false;

    internal::draw_colored_text(content, rect, widget.z_index, c);

    return true;
}

ElementResult text(const Widget& widget, const TranslatableString& content,
                   ui::theme::Usage color_usage, bool draw_background

) {
    Rectangle rect = widget.get_rect();

    // No need to render if text is empty
    if (content.empty()) return false;
    if (draw_background) {
        internal::draw_rect_color(rect, widget.z_index, {0, 0, 0, 180}, false);
        // Disable the dark text since it doesnt align correctly
        // internal::draw_text(text_lookup(content.c_str()),
        // rect::expand_px(rect, 2.f), widget.z_index,
        // ui::theme::Usage::DarkFont);
    }
    internal::draw_text(content, rect, widget.z_index, color_usage);

    return true;
}

ElementResult scroll_window(
    const Widget& widget, Rectangle view,
    const std::function<void(ScrollWindowResult)>& children) {
    //
    auto state = context->widget_init<ui::ScrollViewState>(
        ui::MK_UUID(widget.id, widget.id));

    //
    focus::active_if_mouse_inside(widget, view);
    if (focus::is_hot(widget.id)) {
        state->y_offset = state->y_offset + context->yscrolled;
        float scale = 1.0f;
        state->y_offset =
            fmin(0.f, fmax(-1.f * scale * widget.rect.height, state->y_offset));
    }

    //
    internal::draw_rect(view, widget.z_index, ui::theme::Secondary);
    callback_registry.register_call(
        context,
        [=]() {
            context->scissor_on(view);
            children(
                ScrollWindowResult{rect::offset_y(widget.rect, state->y_offset),
                                   widget.z_index - 1});
            context->scissor_off();
        },
        widget.z_index);
    return ElementResult{true,
                         ScrollWindowResult{widget.rect, widget.z_index - 1}};
}

ElementResult button(const Widget& widget, const TranslatableString& content,
                     bool background, bool draw_background_when_hot) {
    Rectangle rect = widget.get_rect();

    //
    if (internal::should_exit_early(widget)) return false;
    //
    focus::active_if_mouse_inside(widget);
    focus::try_to_grab(widget);
    internal::draw_focus_ring(widget, true);
    focus::handle_tabbing(widget);

    if (background || (draw_background_when_hot && focus::is_hot(widget.id))) {
        // We dont allow customization because
        // you should be using the themed colors
        auto color_usage = ui::theme::Usage::Primary;
        if (focus::is_hot(widget.id)) {
            color_usage = ui::theme::Usage::Secondary;
        }
        internal::draw_rect(rect, widget.z_index, color_usage, true);
    }

    const auto _press_logic = [&]() -> bool {
        if (focus::matches(widget.id) &&
            context->pressed(InputName::WidgetPress)) {
            focus::set(widget.id);
            return true;
        }
        if (focus::is_mouse_click(widget)) {
            focus::set(widget.id);
            return true;
        }
        return false;
    };

    text(widget, content);

    return _press_logic();
}

ElementResult image(const Widget& widget, const std::string& texture_name) {
    Rectangle rect = widget.get_rect();

    if (internal::should_exit_early(widget)) return false;
    const raylib::Texture texture = TextureLibrary::get().get(texture_name);
    const vec2 tex_size = {(float) texture.width, (float) texture.height};
    const vec2 button_size = {rect.width, rect.height};
    internal::draw_image({rect.x, rect.y}, texture,
                         calculateScale(button_size, tex_size), widget.z_index);
    return true;
}

ElementResult image_button(const Widget& widget,
                           const std::string& texture_name) {
    Rectangle rect = widget.get_rect();

    if (internal::should_exit_early(widget)) return false;
    //
    focus::active_if_mouse_inside(widget);
    focus::try_to_grab(widget);
    internal::draw_focus_ring(widget);
    focus::handle_tabbing(widget);

    if (focus::is_hot(widget.id)) {
        focus::set(widget.id);
    }

    const raylib::Texture texture = TextureLibrary::get().get(texture_name);
    const vec2 tex_size = {(float) texture.width, (float) texture.height};
    const vec2 button_size = {rect.width, rect.height};
    internal::draw_image({rect.x, rect.y}, texture,
                         calculateScale(button_size, tex_size), widget.z_index);

    const auto _press_logic = [&]() -> bool {
        if (focus::matches(widget.id) &&
            context->pressed(InputName::WidgetPress)) {
            return true;
        }
        if (focus::is_mouse_click(widget)) {
            return true;
        }
        return false;
    };

    return _press_logic();
}

ElementResult checkbox(const Widget& widget, const CheckboxData& data) {
    // TODO add focus on hover
    //
    if (internal::should_exit_early(widget)) return false;
    //
    auto state = context->widget_init<ui::CheckboxState>(
        ui::MK_UUID(widget.id, widget.id));
    state->on = data.selected;
    state->on.changed_since = false;

    if (data.content_is_icon) {
        if (image_button(widget, data.content)) {
            state->on = !state->on;
        }
    } else {
        if (button(widget, NO_TRANSLATE(""), data.background)) {
            state->on = !state->on;
        }
        const std::string default_label = state->on ? "  X" : " ";
        const std::string label =
            data.content.empty() ? default_label : data.content;

        text(widget, NO_TRANSLATE(label));
    }

    return ElementResult{state->on.changed_since, (bool) state->on};
}

ElementResult slider(const Widget& widget, const SliderData& data) {
    if (internal::should_exit_early(widget)) return false;
    //
    // TODO be able to scroll this bar with the scroll wheel
    auto state = context->widget_init<ui::SliderState>(
        ui::MK_UUID(widget.id, widget.id));
    bool vertical = data.vertical;
    bool changed_previous_frame = state->value.changed_since;
    state->value = data.value;
    state->value.changed_since = false;

    focus::active_if_mouse_inside(widget);
    focus::try_to_grab(widget);
    internal::draw_focus_ring(widget);
    focus::handle_tabbing(widget);

    if (focus::is_hot(widget.id)) {
        focus::set(widget.id);
    }

    {
        internal::draw_rect(widget.get_rect(), widget.z_index,
                            ui::theme::Usage::Primary);  // Slider Rail

        // slide
        Rectangle rect(widget.get_rect());
        const float maxScale = 0.8f;
        const float pos_offset =
            state->value *
            (vertical ? rect.height * maxScale : rect.width * maxScale);

        rect = {
            vertical ? rect.x : rect.x + pos_offset,
            vertical ? rect.y + pos_offset : rect.y,
            vertical ? rect.width : rect.width / 5.f,
            vertical ? rect.height / 5.f : rect.height,
        };

        internal::draw_rect(rect, widget.z_index, ui::theme::Usage::Secondary);
    }

    {
        float mnf = 0.f;
        float mxf = 1.f;

        bool value_changed = false;
        if (focus::matches(widget.id)) {
            if (context->is_held_down(InputName::ValueRight)) {
                state->value = state->value + 0.005f;
                if (state->value > mxf) state->value = mxf;

                value_changed = true;
            }
            if (context->is_held_down(InputName::ValueLeft)) {
                state->value = state->value - 0.005f;
                if (state->value < mnf) state->value = mnf;
                value_changed = true;
            }
        }

        if (focus::is_active(widget.id)) {
            focus::set(widget.id);
            float v;
            if (vertical) {
                v = (focus::mouse_info.pos.y - widget.get_rect().y) /
                    widget.get_rect().height;
            } else {
                v = (focus::mouse_info.pos.x - widget.get_rect().x) /
                    widget.get_rect().width;
            }
            if (v < mnf) v = mnf;
            if (v > mxf) v = mxf;
            if (v != state->value) {
                state->value = v;
                value_changed = true;
            }
        }
        state->value.changed_since = value_changed;
    };

    // TODO this name still true?
    return ElementResult{changed_previous_frame, state->value};
}

ElementResult dropdown(const Widget& widget, DropdownData data) {
    if (internal::should_exit_early(widget)) return false;
    //
    if (data.options.empty()) {
        log_warn("the options passed to dropdown were empty");
        return ElementResult{false, 0};
    }

    //
    // TODO right now you can change values through tab or through
    // arrow keys, maybe we should only allow arrows
    // and then tab just switches to the next non dropdown widget
    // NOTE: originally we only did this when the dropdown wasnt already
    // open but we should be safe to always do this
    // 1. doesnt toggle on, sets on directly
    // 2. open should have children focused anyway
    // 3. we dont eat the input, so it doesnt break the button_list
    // value up/down

    auto state = context->widget_init<ui::DropdownState>(
        ui::MK_UUID(widget.id, widget.id));
    state->selected = data.initial;
    state->selected.changed_since = false;

    // if you arrow up out, then tab to previous
    if (state->focused < 0) {
        state->on = false;
        focus::set_previous();
    }

    if (button(widget, NO_TRANSLATE(""), true)) {
        state->on = !state->on;
    }

    std::vector<int> option_ids;
    std::vector<Widget> option_widgets;
    if (state->on) {
        {
            Rectangle rect = widget.get_rect();
            for (int i = 0; i < (int) data.options.size(); i++) {
                rect.y += rect.height;
                Widget option_widget(widget);
                // this should be above the button's index
                option_widget.z_index--;
                // Needs to be unique on the screen
                auto uuid = ui::MK_UUID_LOOP(widget.id, widget.id, i);
                option_widget.id = (int) (size_t) uuid;
                option_widget.rect = (Rectangle(rect));

                option_widgets.push_back(option_widget);
                option_ids.push_back(option_widget.id);
            }
        }

        bool has_focus = focus::matches(widget.id);
        for (auto id : option_ids) {
            has_focus |= focus::matches(id);
        }

        state->on = has_focus;
    }

    // If you arrow down out, then tab to next item
    if (state->focused >= (int) data.options.size()) {
        state->on = false;
        focus::set(focus::ROOT_ID);
    }

    if (state->on) {
        // TODO should tabbing go to the next item? instead of the next
        // option?

        // Note: we do this this way because pressed() eats the input,
        // so we can merge it with the below unless we switch to no eat
        if (context->pressed(InputName::WidgetNext)) {
            if (context->is_held_down(InputName::WidgetMod)) {
                state->focused = state->focused - 1;
            } else {
                state->focused = state->focused + 1;
            }
        }

        if (context->pressed(InputName::ValueUp)) {
            state->focused = state->focused - 1;
        }

        if (context->pressed(InputName::ValueDown)) {
            state->focused = state->focused + 1;
        }

        int i = -1;
        for (const Widget& option_widget : option_widgets) {
            i++;

            // If this was the one that is selected, set the focus to it
            if (state->focused == i) {
                focus::set(option_widget.id);
            }

            if (button(option_widget, TODO_TRANSLATE(data.options[i],
                                                     TodoReason::Recursion))) {
                state->selected = i;
                state->on = false;
            }
        }
    } else {
        state->focused = state->selected;
    }

    text(widget,
         TODO_TRANSLATE(data.options[state->selected], TodoReason::Recursion));

    return ElementResult{state->selected.changed_since, (int) state->selected};
}

ElementResult control_input_field(const Widget& widget,
                                  const TextfieldData& data) {
    if (internal::should_exit_early(widget)) return false;
    //
    bool valid = false;
    AnyInput ai;

    if (focus::matches(widget.id)) {
        valid = true;
        ai = context->last_input_pressed();

        if (std::holds_alternative<int>(ai)) {
            if (std::get<int>(ai) == 0) {
                valid = false;
            }
        } else if (std::holds_alternative<GamepadButton>(ai)) {
            if (std::get<GamepadButton>(ai) == raylib::GAMEPAD_BUTTON_UNKNOWN) {
                valid = false;
            }
        } else if (std::holds_alternative<GamepadAxisWithDir>(ai)) {
            log_warn("control_input_field doesnt support gamepad axis");
            valid = false;
        }
        context->clear_last_input();
    }

    focus::active_if_mouse_inside(widget);
    focus::try_to_grab(widget);
    // TODO change focus color based on is invalid...
    internal::draw_focus_ring(widget);
    focus::handle_tabbing(widget);

    if (focus::is_mouse_click(widget)) {
        focus::set(widget.id);
    }

    internal::draw_rect(widget.get_rect(), widget.z_index,
                        focus::is_active_and_hot(widget.id)
                            ? ui::theme::Usage::Secondary
                            : ui::theme::Usage::Primary);

    if (data.content_is_icon) {
        image(widget, data.content);
    } else {
        text(widget, NO_TRANSLATE(data.content));
    }

    return ElementResult{valid, ai};
}

ElementResult textfield(const Widget& widget, const TextfieldData& data) {
    if (internal::should_exit_early(widget)) return false;
    //
    auto state = context->widget_init<ui::TextfieldState>(
        ui::MK_UUID(widget.id, widget.id));
    state->buffer = data.content;
    state->buffer.changed_since = false;

    // editable text field...

    TextfieldValidationDecisionFlag validationFlag =
        data.validationFunction ? data.validationFunction(state->buffer.asT())
                                : TextfieldValidationDecisionFlag::None;
    bool is_invalid = validation_test(validationFlag,
                                      TextfieldValidationDecisionFlag::Invalid);

    focus::active_if_mouse_inside(widget);
    focus::try_to_grab(widget);
    // TODO change focus color based on is invalid...
    internal::draw_focus_ring(widget);
    focus::handle_tabbing(widget);

    if (focus::is_mouse_click(widget)) {
        focus::set(widget.id);
    }

    // render
    {
        internal::draw_rect(widget.get_rect(), widget.z_index,
                            focus::is_active_and_hot(widget.id)
                                ? ui::theme::Usage::Secondary
                                : ui::theme::Usage::Primary);

        bool shouldWriteCursor = focus::matches(widget.id) && state->showCursor;
        const std::string focusStr = shouldWriteCursor ? "_" : "";
        const std::string focused_content =
            fmt::format("{}{}", state->buffer.asT(), focusStr);

        auto text_color =
            is_invalid ? ui::theme::Usage::Error : ui::theme::Usage::Font;
        text(widget, NO_TRANSLATE(focused_content), text_color);
    }

    {
        state->cursorBlinkTime = state->cursorBlinkTime + 1;
        if (state->cursorBlinkTime > 60) {
            state->cursorBlinkTime = 0;
            state->showCursor = !state->showCursor;
        }

        bool changed = false;

        if (focus::matches(widget.id)) {
            // TODO what does this mean
            if (context->keychar != int()) {
                if (validation_test(
                        validationFlag,
                        TextfieldValidationDecisionFlag::StopNewInput)) {
                    log_warn("tried to type {} but hit validation error",
                             context->keychar);
                } else {
                    state->buffer.asT().append(
                        std::string(1, (char) context->keychar));
                    changed = true;
                }
            }

            if (context->pressed(InputName::WidgetBackspace) ||
                context->is_held_down_debounced(InputName::WidgetBackspace)) {
                if (state->buffer.asT().size() > 0) {
                    state->buffer.asT().pop_back();
                }
                changed = true;
            }

            if (context->is_held_down(InputName::WidgetCtrl)) {
                if (context->pressed(InputName::WidgetPaste)) {
                    auto clipboard = ext::get_clipboard_text();

                    // make a copy so we can text validation
                    std::string post_paste(state->buffer.asT());
                    post_paste.append(clipboard);

                    // TODO we should paste the amount that fits. but we
                    // dont know what the max length actually is, so we
                    // would need to remove one letter at a time until we
                    // hit something valid.

                    // TODO we probably should give some kind of visual
                    // error to the user that you cant paste right now
                    bool should_append = !validation_test(
                        validationFlag,
                        TextfieldValidationDecisionFlag::StopNewInput);

                    // commit the copy-paste
                    if (should_append) state->buffer.asT() = post_paste;
                }
                changed = true;
            }
        }

        state->buffer.changed_since = changed;
    }

    return ElementResult{state->buffer.changed_since, state->buffer.asT()};
}

}  // namespace ui
