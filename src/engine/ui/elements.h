

#pragma once

#include "../font_sizer.h"
#include "../ui_context.h"
#include "../uuid.h"
#include "parsing.h"

namespace elements {

class CallbackRegistry {
   public:
    inline void register_call(std::function<void(void)> cb, int z_index = 0) {
        callbacks.emplace_back(ScheduledCall{z_index, cb});
        insert_sorted();
    }

    void execute_callbacks() {
        for (const auto& entry : callbacks) {
            entry.callback();
        }
        callbacks.clear();
    }

   private:
    struct ScheduledCall {
        int z_index;
        std::function<void(void)> callback;
    };

    std::vector<ScheduledCall> callbacks;

    void insert_sorted() {
        if (callbacks.size() <= 1) {
            return;
        }

        auto it = callbacks.end() - 1;
        while (it != callbacks.begin() && (it - 1)->z_index < it->z_index) {
            std::iter_swap(it - 1, it);
            --it;
        }
    }
};
static CallbackRegistry callback_registry;
static std::shared_ptr<ui::UIContext> context;

inline float calculateScale(const vec2& rect_size, const vec2& image_size) {
    float scale_x = rect_size.x / image_size.x;
    float scale_y = rect_size.y / image_size.y;
    return std::min(scale_x, scale_y);
}

struct DropdownData {
    std::vector<std::string> options;
    int initial = 0;
};

struct TextfieldData {
    std::string content;
};

typedef std::variant<std::string, bool, float, TextfieldData, DropdownData>
    InputDataSource;

struct Widget {
    LayoutBox layout_box;
    int id;
    int z_index = 0;

    Widget(const LayoutBox& lb, int i) : layout_box(lb), id(i) {}

    std::optional<ui::theme::Usage> get_usage_color_maybe(
        const std::string& type) const {
        auto box = layout_box;
        auto theme = box.style.lookup_theme(type);
        return theme;
    }

    ui::theme::Usage get_usage_color(const std::string type) const {
        auto theme = get_usage_color_maybe(type);
        if (theme.has_value()) return theme.value();
        log_warn(
            "trying to get usage color from style for {} but it was missing "
            "from {}",
            type, layout_box.node.tag);
        return ui::theme::Usage::Primary;
    }

    const std::string& get_content_from_node() const {
        return layout_box.node.content;
    }

    const std::optional<std::string> get_possible_background_image() const {
        return layout_box.style.lookup_s("background-image");
    }

    bool has_background_color() const {
        return get_usage_color_maybe("background-color").has_value();
    }

    Rectangle get_rect() const { return layout_box.dims.content; }
    void set_rect(Rectangle r) { layout_box.dims.content = r; }

    const std::string& attr(const std::string& name,
                            const std::string& def) const {
        return layout_box.attr(name, def);
    }
};

namespace focus {
static const int ROOT_ID = -1;
static const int FAKE_ID = -2;
static int focus_id = ROOT_ID;
static int last_processed = ROOT_ID;

static int hot_id = ROOT_ID;
static int active_id = ROOT_ID;
static MouseInfo mouse_info;

static std::set<int> ids;

inline bool is_mouse_inside(const Rectangle& rect) {
    auto mouse = mouse_info.pos;
    return mouse.x >= rect.x && mouse.x <= rect.x + rect.width &&
           mouse.y >= rect.y && mouse.y <= rect.y + rect.height;
}

inline bool is_mouse_down_in_box(const Rectangle& rect) {
    return is_mouse_inside(rect) && mouse_info.leftDown;
}

inline int get() { return focus_id; }
inline void set(int id) { focus_id = id; }
inline bool matches(int id) { return get() == id; }
inline bool up_for_grabs() { return matches(ROOT_ID); }
inline void try_to_grab(const Widget& widget) {
    ids.insert(widget.id);
    if (up_for_grabs()) {
        set(widget.id);
    }
}

inline void set_hot(int id) { hot_id = id; }
inline void set_active(int id) { active_id = id; }
inline bool is_hot(int id) { return hot_id == id; }
inline bool is_active(int id) { return active_id == id; }
inline bool is_active_or_hot(int id) { return is_hot(id) || is_active(id); }
inline bool is_active_and_hot(int id) { return is_hot(id) && is_active(id); }

inline void active_if_mouse_inside(const Widget& widget) {
    bool inside = is_mouse_inside(widget.get_rect());
    if (inside) {
        set_hot(widget.id);
        if (is_active(ROOT_ID) && mouse_info.leftDown) {
            set_active(widget.id);
        }
    }
}

inline bool is_mouse_click(const Widget& widget) {
    bool let_go_of_mouse = !mouse_info.leftDown;
    return let_go_of_mouse && is_active_and_hot(widget.id);
}

inline void handle_tabbing(const Widget& widget) {
    // TODO How do we handle something that wants to use
    // Widget Value Down/Up to control the value?
    // Do we mark the widget type with "nextable"? (tab will always work but
    // not very discoverable
    if (matches(widget.id)) {
        if (
            //
            context->pressed(InputName::WidgetNext) ||
            context->pressed(InputName::ValueDown)
            // TODO add support for holding down tab
            // get().is_held_down_debounced(InputName::WidgetNext) ||
            // get().is_held_down_debounced(InputName::ValueDown)
        ) {
            set(ROOT_ID);
            if (context->is_held_down(InputName::WidgetMod)) {
                set(last_processed);
            }
        }
        if (context->pressed(InputName::ValueUp)) {
            set(last_processed);
        }
        if (context->pressed(InputName::WidgetBack)) {
            set(last_processed);
        }
    }
    // before any returns
    last_processed = widget.id;
}

inline void begin() {
    mouse_info = get_mouse_info();
    hot_id = ROOT_ID;
}

inline void end() {
    if (up_for_grabs()) return;

    if (mouse_info.leftDown) {
        if (is_active(ROOT_ID)) {
            set_active(FAKE_ID);
        }
    } else {
        set_active(ROOT_ID);
    }

    if (!ids.contains(focus_id)) {
        focus_id = ROOT_ID;
    }
    ids.clear();
}

}  // namespace focus

inline void begin(std::shared_ptr<ui::UIContext> ui_context, float dt) {
    context = ui_context;
    focus::begin();
    //
    context->begin(dt);
}

inline void end() {
    callback_registry.execute_callbacks();
    context->render_all();
    focus::end();

    context->cleanup();
}

struct ElementResult {
    // no explicit on purpose
    ElementResult(bool val) : result(val) {}
    ElementResult(bool val, bool d) : result(val), data(d) {}
    ElementResult(bool val, int d) : result(val), data(d) {}
    ElementResult(bool val, float d) : result(val), data(d) {}
    ElementResult(bool val, const std::string& d) : result(val), data(d) {}

    template<typename T>
    T as() const {
        return std::get<T>(data);
    }

    operator bool() const { return result; }

   private:
    bool result = false;
    std::variant<bool, int, float, std::string> data = 0;
};

namespace internal {

inline void draw_text(const std::string& content, Rectangle parent, int z_index,
                      ui::theme::Usage color_usage = ui::theme::Usage::Font) {
    callback_registry.register_call(
        [=]() {
            context->_draw_text(parent, text_lookup(content.c_str()),
                                color_usage);
        },
        z_index);
}

inline void draw_rect(Rectangle rect, int z_index,
                      ui::theme::Usage color_usage = ui::theme::Usage::Font) {
    callback_registry.register_call(
        [=]() { context->draw_widget_rect(rect, color_usage); }, z_index);
}

inline void draw_image(vec2 pos, raylib::Texture texture, float scale,
                       int z_index) {
    callback_registry.register_call(
        [=]() { context->draw_image(texture, pos, 0, scale); }, z_index);
}

inline void draw_focus_ring(const Widget& widget) {
    if (!focus::matches(widget.id)) return;
    Rectangle rect = widget.get_rect();
    float pixels = WIN_HF() * 0.003f;
    rect = expand(rect, {pixels, pixels, pixels, pixels});
    internal::draw_rect(rect, widget.z_index + 1, ui::theme::Usage::Accent);
}

}  // namespace internal

inline ElementResult text(const Widget& widget, const std::string& content,
                          Rectangle parent,
                          ui::theme::Usage color_usage = ui::theme::Usage::Font

) {
    // No need to render if text is empty
    if (content.empty()) return false;
    internal::draw_text(text_lookup(content.c_str()), parent, widget.z_index,
                        color_usage);
    return true;
}

inline ElementResult div(const Widget& widget) {
    Rectangle rect = widget.get_rect();
    if (widget.has_background_color()) {
        auto color_usage = widget.get_usage_color("background-color");
        internal::draw_rect(rect, widget.z_index, color_usage);
    }
    return true;
}

inline ElementResult button(const Widget& widget, bool background = true) {
    Rectangle rect = widget.get_rect();

    //
    focus::active_if_mouse_inside(widget);
    focus::try_to_grab(widget);
    internal::draw_focus_ring(widget);
    focus::handle_tabbing(widget);

    auto image = widget.get_possible_background_image();
    if (image.has_value()) {
        if (focus::is_hot(widget.id)) {
            auto color_usage = ui::theme::Usage::Accent;
            internal::draw_rect(rect, widget.z_index, color_usage);
        }
        const raylib::Texture texture =
            TextureLibrary::get().get(image.value());
        const vec2 tex_size = {(float) texture.width, (float) texture.height};
        const vec2 button_size = {rect.width, rect.height};
        internal::draw_image({rect.x, rect.y}, texture,
                             elements::calculateScale(button_size, tex_size),
                             widget.z_index);

        // Force no background for now
        background = false;
    }

    if (background) {
        auto color_usage = widget.get_usage_color("background-color");

        // TODO add style for 'hover' state
        if (focus::is_hot(widget.id)) {
            color_usage = ui::theme::Usage::Accent;
        }
        internal::draw_rect(rect, widget.z_index, color_usage);
    }

    const auto _press_logic = [&]() -> bool {
        if (focus::matches(widget.id) &&
            context->pressed(InputName::WidgetPress)) {
            return true;
        }
        if (focus::is_mouse_click(widget)) {
            if (!widget.layout_box.node.attrs.contains("id")) {
                log_warn("you have a button without an id {} {}",
                         widget.layout_box.node.tag,
                         widget.layout_box.node.children[0].content);
                return false;
            }
            return true;
        }
        return false;
    };

    return _press_logic();
}

// Returns true if the checkbox changed
inline ElementResult checkbox(const Widget& widget,
                              const TextfieldData& data = {}) {
    auto state = context->widget_init<ui::CheckboxState>(
        ui::MK_UUID(widget.id, widget.id));
    state->on.changed_since = false;

    if (button(widget, true)) {
        state->on = !state->on;
    }

    const std::string default_label = state->on ? "  X" : " ";
    const std::string label =
        data.content.empty() ? default_label : data.content;

    text(widget, label, widget.get_rect());

    return ElementResult{state->on.changed_since, state->on};
}

inline ElementResult slider(const Widget& widget, bool vertical = false) {
    // TODO be able to scroll this bar with the scroll wheel
    auto state = context->widget_init<ui::SliderState>(
        ui::MK_UUID(widget.id, widget.id));
    bool changed_previous_frame = state->value.changed_since;
    state->value.changed_since = false;

    focus::active_if_mouse_inside(widget);
    focus::try_to_grab(widget);
    internal::draw_focus_ring(widget);
    focus::handle_tabbing(widget);

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

        internal::draw_rect(rect, widget.z_index, ui::theme::Usage::Accent);
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

inline ElementResult dropdown(const Widget& widget, DropdownData data) {
    if (data.options.empty()) {
        log_warn("the options passed to dropdown were empty");
        return false;
    }

    // TODO when you tab to the dropdown
    // it would be nice if it opened

    // TODO check if this is still true, theres ltos to check about the tab
    // states on this, also check backwards tabs
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

    if (button(widget, true)) {
        state->on = !state->on;
    }

    if (state->on) {
        Rectangle rect = widget.get_rect();
        int i = -1;
        for (const auto& option : data.options) {
            rect.y += rect.height;
            i++;

            Widget option_widget(widget);
            // this should be above the button's index
            option_widget.z_index--;

            // Needs to be unique on the screen
            auto uuid = ui::MK_UUID_LOOP(widget.id, widget.id, i);
            option_widget.id = (int) (size_t) uuid;

            option_widget.set_rect(Rectangle(rect));

            if (button(option_widget)) {
                state->selected = i;
                state->on = false;
            }

            text(option_widget, option, rect);
        }
    }

    text(widget, data.options[state->selected], widget.get_rect());

    return ElementResult{state->selected.changed_since, state->selected};
}

inline ElementResult textfield(const Widget& widget,
                               const TextfieldData& data) {
    auto state = context->widget_init<ui::TextfieldState>(
        ui::MK_UUID(widget.id, widget.id));
    state->buffer = data.content;
    state->buffer.changed_since = false;

    bool disabled = widget.attr("disabled", "false") == "true";
    if (disabled) {
        text(widget, state->buffer, widget.get_rect());
        return ElementResult{false, state->buffer};
    }

    // editable text field...

    bool is_invalid = false;  // TODO

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
        // TODO support the css color?
        //    widget.get_usage_color("background-color")
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
        text(widget, focused_content, widget.get_rect(), text_color);
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
                // TODO check validation

                state->buffer.asT().append(
                    std::string(1, (char) context->keychar));
                changed = true;
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

                    // TODO run validation

                    // TODO we should paste the amount that fits. but we
                    // dont know what the max length actually is, so we
                    // would need to remove one letter at a time until we
                    // hit something valid.

                    // TODO we probably should give some kind of visual
                    // error to the user that you cant paste right now
                    // bool should_append = !validation_test(
                    // vflag, TextfieldValidationDecisionFlag::StopNewInput);

                    bool should_append = true;

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

}  // namespace elements
