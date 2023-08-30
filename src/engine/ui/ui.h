
#pragma once

#include "../gamepad_axis_with_dir.h"
#include "raylib.h"
//
#include <codecvt>
#include <locale>
#include <string>
#include <variant>

#include "../../preload.h"
#include "../font_util.h"
#include "../texture_library.h"
#include "../uuid.h"
#include "autolayout.h"
#include "color.h"
#include "context.h"
#include "state.h"
#include "theme.h"
#include "widget.h"

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

namespace rect {

inline Rectangle lpad(const Rectangle& r, float pct);
inline Rectangle rpad(const Rectangle& r, float pct);
inline Rectangle tpad(const Rectangle& r, float pct);
inline Rectangle bpad(const Rectangle& r, float pct);

inline raylib::Rectangle expand(const raylib::Rectangle& a, const vec4& b) {
    return (Rectangle){a.x - b.x,            //
                       a.y - b.y,            //
                       a.width + b.x + b.z,  //
                       a.height + b.y + b.w};
}

inline Rectangle expand_pct(const Rectangle& a, float pct) {
    return expand(a, {
                         a.width * pct,
                         a.width * pct,
                         a.height * pct,
                         a.height * pct,
                     });
}

inline Rectangle offset_y(const Rectangle& a, float y) {
    return Rectangle{a.x,      //
                     a.y + y,  //
                     a.width,  //
                     a.height};
}

inline std::array<Rectangle, 2> vsplit(const Rectangle& a, float pct,
                                       float padding_right = 0) {
    float ratio = pct / 100.f;
    Rectangle left = {a.x, a.y, a.width * ratio, a.height};
    if (padding_right > 0) {
        left = rpad(left, 100 - padding_right);
    }
    Rectangle right = {a.x + (a.width * ratio), a.y, a.width * (1.f - ratio),
                       a.height};
    return {left, right};
}

inline std::array<Rectangle, 2> hsplit(const Rectangle& a, float pct,
                                       float padding_bottom = 0) {
    float ratio = pct / 100.f;
    Rectangle top = {a.x, a.y, a.width, a.height * ratio};
    if (padding_bottom > 0) {
        top = bpad(top, 100 - padding_bottom);
    }
    Rectangle bottom = {a.x, a.y + (a.height * ratio), a.width,
                        a.height * (1.f - ratio)};
    return {top, bottom};
}

inline Rectangle lpad(const Rectangle& r, float pct) {
    auto [_, b] = vsplit(r, pct);
    return b;
}

inline Rectangle rpad(const Rectangle& r, float pct) {
    auto [a, _] = vsplit(r, pct);
    return a;
}

inline Rectangle tpad(const Rectangle& r, float pct) {
    auto [_, b] = hsplit(r, pct);
    return b;
}

inline Rectangle bpad(const Rectangle& r, float pct) {
    auto [a, _] = hsplit(r, pct);
    return a;
}

inline Rectangle vpad(const Rectangle& r, float pct) {
    return tpad(bpad(r, pct), pct);
}

inline Rectangle hpad(const Rectangle& r, float pct) {
    return rpad(lpad(r, pct), pct);
}

template<size_t N>
inline std::array<Rectangle, N> vsplit(const Rectangle& a,
                                       float padding_right = 0) {
    std::array<Rectangle, N> rectangles;
    float step = a.width / N;

    float x = a.x;
    float y = a.y;
    float width = step;
    float height = a.height;
    for (size_t i = 0; i < N; ++i) {
        rectangles[i] = Rectangle{x, y, width, height};
        if (padding_right > 0) {
            rectangles[i] = rect::rpad(rectangles[i], 100 - padding_right);
        }
        x += step;
        y += 0;
    }
    return rectangles;
}

template<>
inline std::array<Rectangle, 2> vsplit(const Rectangle& a,
                                       float padding_right) {
    return vsplit(a, 50.f, padding_right);
}

template<size_t N>
inline std::array<Rectangle, N> hsplit(const Rectangle& a,
                                       float padding_bottom = 0) {
    std::array<Rectangle, N> rectangles;
    float step = a.height / N;

    float x = a.x;
    float y = a.y;
    float width = a.width;
    float height = step;
    for (size_t i = 0; i < N; ++i) {
        rectangles[i] = Rectangle{x, y, width, height};
        if (padding_bottom > 0) {
            rectangles[i] = rect::bpad(rectangles[i], 100 - padding_bottom);
        }
        x += 0;
        y += step;
    }
    return rectangles;
}

template<>
inline std::array<Rectangle, 2> hsplit(const Rectangle& a,
                                       float padding_bottom) {
    return hsplit(a, 50.f, padding_bottom);
}

}  // namespace rect

static std::shared_ptr<ui::UIContext> context;

class CallbackRegistry {
   public:
    inline void register_call(std::function<void(void)> cb, int z_index = 0) {
        if (context->should_immediate_draw) {
            cb();
            return;
        }
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

inline float calculateScale(const vec2& rect_size, const vec2& image_size) {
    float scale_x = rect_size.x / image_size.x;
    float scale_y = rect_size.y / image_size.y;
    return std::min(scale_x, scale_y);
}

struct DropdownData {
    std::vector<std::string> options;
    int initial = 0;
};

struct CheckboxData {
    bool selected = false;
    std::string content = "";
};

struct TextfieldData {
    std::string content = "";
    TextFieldValidationFn validationFunction = {};
};

struct SliderData {
    bool vertical = false;
    float value = 0.f;
};

// TODO theoretically this should probably match the types that go into init<>
typedef std::variant<std::string, bool, float, CheckboxData, TextfieldData,
                     SliderData, DropdownData>
    InputDataSource;

static std::atomic_int WIDGET_ID = 0;

struct Widget {
    int id = 0;
    int z_index = 0;
    Rectangle rect = {0, 0, WIN_WF(), WIN_HF()};

    Widget(Rectangle r) : id(WIDGET_ID++), z_index(0), rect(r) {}
    Widget(Rectangle r, int z_) : id(WIDGET_ID++), z_index(z_), rect(r) {}

    Rectangle get_rect() const { return rect; }
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
inline bool was_last(int id) { return id == last_processed; }

inline void active_if_mouse_inside(const Widget& widget,
                                   std::optional<Rectangle> view = {}) {
    bool inside =
        is_mouse_inside(view.has_value() ? view.value() : widget.get_rect());
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

inline void reset() {
    focus_id = ROOT_ID;
    last_processed = ROOT_ID;

    hot_id = ROOT_ID;
    active_id = ROOT_ID;
    ids.clear();
}

inline void begin() {
    mouse_info = get_mouse_info();
    hot_id = ROOT_ID;
    WIDGET_ID = 0;
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

struct ScrollWindowResult {
    Rectangle sv;
    int z_index;
};

struct ElementResult {
    // no explicit on purpose
    ElementResult(bool val) : result(val) {}
    ElementResult(bool val, bool d) : result(val), data(d) {}
    ElementResult(bool val, int d) : result(val), data(d) {}
    ElementResult(bool val, float d) : result(val), data(d) {}
    ElementResult(bool val, Rectangle d) : result(val), data(d) {}
    ElementResult(bool val, const std::string& d) : result(val), data(d) {}
    ElementResult(bool val, const AnyInput& d) : result(val), data(d) {}
    ElementResult(bool val, const ScrollWindowResult& d)
        : result(val), data(d) {}

    template<typename T>
    T as() const {
        return std::get<T>(data);
    }

    operator bool() const { return result; }

   private:
    bool result = false;
    std::variant<bool, int, float, std::string, AnyInput, Rectangle,
                 ScrollWindowResult>
        data = 0;
};

namespace internal {

inline bool should_exit_early(const Widget& widget) {
    if (!context->scissor_box.has_value()) return false;
    return !raylib::CheckCollisionRecs(widget.rect,
                                       context->scissor_box.value());
}

// TODO replace with either nothing or using context's
inline ui::UITheme active_theme() { return ui::DEFAULT_THEME; }

inline void draw_text(const std::string& content, Rectangle parent, int z_index,
                      ui::theme::Usage color_usage = ui::theme::Usage::Font) {
    callback_registry.register_call(
        [=]() {
            // TODO dynamically choose font based on current set language
            // TODO what happens if the two players are using different
            // languages
            auto font = Preload::get().font;

            // TODO we also probably want to preload with the set of all strings
            // so the font is already loaded for strings we know will be there
            //
            // then we only have to dynamically fetch for user strings
            //
            // load_font_for_string(
            // content, "./resources/fonts/NotoSansKR.ttf");
            // content, "./resources/fonts/Sazanami-Hanazono-Mincho.ttf");

            auto rect = parent;
            auto spacing = 0.f;
            // TODO move the generator out of context
            auto font_size = context->get_font_size(content, rect.width,
                                                    rect.height, spacing);

            DrawTextEx(font,                //
                       content.c_str(),     //
                       {rect.x, rect.y},    //
                       font_size, spacing,  //
                       active_theme().from_usage(color_usage));
        },
        z_index);
}

inline void draw_rect_color(Rectangle rect, int z_index, Color c) {
    callback_registry.register_call(
        [=]() { DrawRectangleRounded(rect, 0.15f, 4, c); }, z_index);
}

inline void draw_rect(
    Rectangle rect, int z_index,
    ui::theme::Usage color_usage = ui::theme::Usage::Primary) {
    draw_rect_color(rect, z_index, active_theme().from_usage(color_usage));
}

inline void draw_image(vec2 pos, raylib::Texture texture, float scale,
                       int z_index) {
    callback_registry.register_call(
        [=]() {
            raylib::DrawTextureEx(texture, pos, 0 /* rotation */, scale,
                                  WHITE /*tint*/);
        },
        z_index);
}

inline void draw_focus_ring(const Widget& widget) {
    if (!focus::matches(widget.id)) return;
    Rectangle rect = widget.get_rect();
    float pixels = WIN_HF() * 0.003f;
    rect = rect::expand(rect, {pixels, pixels, pixels, pixels});
    internal::draw_rect(rect, widget.z_index + 1, ui::theme::Usage::Accent);
}

}  // namespace internal

inline void begin(std::shared_ptr<ui::UIContext> ui_context, float dt) {
    bool new_screen = context ? context->id != ui_context->id : true;
    context = ui_context;

    if (new_screen) {
        focus::reset();
    }

    focus::begin();
    //
    context->begin(dt);
}

inline void end() {
    callback_registry.execute_callbacks();
    focus::end();

    context->cleanup();
}

inline ElementResult div(const Widget& widget, Color c) {
    Rectangle rect = widget.get_rect();
    internal::draw_rect_color(rect, widget.z_index, c);
    return true;
}

inline ElementResult text(const Widget& widget, const std::string& content,
                          ui::theme::Usage color_usage = ui::theme::Usage::Font

) {
    Rectangle rect = widget.get_rect();

    // No need to render if text is empty
    if (content.empty()) return false;
    internal::draw_text(text_lookup(content.c_str()), rect, widget.z_index,
                        color_usage);

    return true;
}

inline ElementResult window(const Widget& widget) {
    internal::draw_rect(widget.rect, widget.z_index, ui::theme::Primary);
    return ElementResult{true, widget.z_index - 1};
}

inline ElementResult scroll_window(
    const Widget& widget, Rectangle view,
    std::function<void(ScrollWindowResult)> children) {
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

inline ElementResult button(const Widget& widget,
                            const std::string& content = "",
                            bool background = true) {
    Rectangle rect = widget.get_rect();

    //
    if (internal::should_exit_early(widget)) return false;
    //
    focus::active_if_mouse_inside(widget);
    focus::try_to_grab(widget);
    internal::draw_focus_ring(widget);
    focus::handle_tabbing(widget);

    if (background) {
        // TODO Add way to control color
        auto color_usage = ui::theme::Usage::Primary;

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
            return true;
        }
        return false;
    };

    text(widget, content);

    return _press_logic();
}

inline ElementResult image_button(const Widget& widget,
                                  const std::string& texture_name) {
    Rectangle rect = widget.get_rect();

    if (internal::should_exit_early(widget)) return false;
    //
    focus::active_if_mouse_inside(widget);
    focus::try_to_grab(widget);
    internal::draw_focus_ring(widget);
    focus::handle_tabbing(widget);

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

inline ElementResult checkbox(const Widget& widget,
                              const CheckboxData& data = {}) {
    if (internal::should_exit_early(widget)) return false;
    //
    auto state = context->widget_init<ui::CheckboxState>(
        ui::MK_UUID(widget.id, widget.id));
    state->on = data.selected;
    state->on.changed_since = false;

    if (button(widget, "", true)) {
        state->on = !state->on;
    }

    const std::string default_label = state->on ? "  X" : " ";
    const std::string label =
        data.content.empty() ? default_label : data.content;

    text(widget, label);

    return ElementResult{state->on.changed_since, (bool) state->on};
}

inline ElementResult slider(const Widget& widget, const SliderData& data = {}) {
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
    if (internal::should_exit_early(widget)) return false;
    //
    if (data.options.empty()) {
        log_warn("the options passed to dropdown were empty");
        return ElementResult{false, 0};
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

    if (button(widget, "", true)) {
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

            option_widget.rect = (Rectangle(rect));

            if (button(option_widget)) {
                state->selected = i;
                state->on = false;
            }

            text(option_widget, option);
        }
    }

    text(widget, data.options[state->selected]);

    return ElementResult{state->selected.changed_since, (int) state->selected};
}

inline ElementResult control_input_field(const Widget& widget,
                                         const TextfieldData& data = {}) {
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

    text(widget, data.content);

    return ElementResult{valid, ai};
}

inline ElementResult textfield(const Widget& widget,
                               const TextfieldData& data) {
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
        text(widget, focused_content, text_color);
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