

#pragma once

#include "../font_sizer.h"
#include "../ui_context.h"
#include "../uuid.h"
#include "parsing.h"

namespace elements {

inline float calculateScale(const vec2& rect_size, const vec2& image_size) {
    float scale_x = rect_size.x / image_size.x;
    float scale_y = rect_size.y / image_size.y;
    return std::min(scale_x, scale_y);
}

struct Widget {
    LayoutBox layout_box;
    int id;

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
            "trying to get usage color from style for {} but it was missing",
            type);
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

inline void handle_tabbing(std::shared_ptr<ui::UIContext> ui_context,
                           const Widget& widget) {
    // TODO How do we handle something that wants to use
    // Widget Value Down/Up to control the value?
    // Do we mark the widget type with "nextable"? (tab will always work but
    // not very discoverable
    if (matches(widget.id)) {
        if (
            //
            ui_context->pressed(InputName::WidgetNext) ||
            ui_context->pressed(InputName::ValueDown)
            // TODO add support for holding down tab
            // get().is_held_down_debounced(InputName::WidgetNext) ||
            // get().is_held_down_debounced(InputName::ValueDown)
        ) {
            set(ROOT_ID);
            if (ui_context->is_held_down(InputName::WidgetMod)) {
                set(last_processed);
            }
        }
        if (ui_context->pressed(InputName::ValueUp)) {
            set(last_processed);
        }
        if (ui_context->pressed(InputName::WidgetBack)) {
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

namespace internal {

inline void draw_focus_ring(                    //
    std::shared_ptr<ui::UIContext> ui_context,  //
    const Widget& widget                        //
) {
    if (!focus::matches(widget.id)) return;
    Rectangle rect = widget.get_rect();
    float pixels = rect.width * 0.01f;
    rect = expand(rect, {pixels, pixels, pixels, pixels});
    ui_context->draw_widget_rect(rect, ui::theme::Usage::Accent);
}

}  // namespace internal

inline bool text(std::shared_ptr<ui::UIContext> ui_context, Widget,
                 const std::string& content, Rectangle parent,
                 ui::theme::Usage color_usage = ui::theme::Usage::Font

) {
    // No need to render if text is empty
    if (content.empty()) return false;
    ui_context->_draw_text(parent, text_lookup(content.c_str()), color_usage);
    return true;
}

inline bool div(std::shared_ptr<ui::UIContext> ui_context,
                const Widget& widget) {
    Rectangle rect = widget.get_rect();
    if (widget.has_background_color()) {
        auto color_usage = widget.get_usage_color("background-color");
        ui_context->draw_widget_rect(rect, color_usage);
    }
    return true;
}

inline bool button(                             //
    std::shared_ptr<ui::UIContext> ui_context,  //
    const Widget& widget                        //
) {
    bool background = true;
    Rectangle rect = widget.get_rect();

    //
    focus::active_if_mouse_inside(widget);
    focus::try_to_grab(widget);
    internal::draw_focus_ring(ui_context, widget);
    focus::handle_tabbing(ui_context, widget);

    auto image = widget.get_possible_background_image();
    if (image.has_value()) {
        if (focus::is_hot(widget.id)) {
            auto color_usage = ui::theme::Usage::Accent;
            ui_context->draw_widget_rect(rect, color_usage);
        }
        const raylib::Texture texture =
            TextureLibrary::get().get(image.value());
        const vec2 tex_size = {(float) texture.width, (float) texture.height};
        const vec2 button_size = {rect.width, rect.height};
        ui_context->draw_image(texture, {rect.x, rect.y}, 0,
                               elements::calculateScale(button_size, tex_size));

        // Force no background for now
        background = false;
    }

    if (background) {
        auto color_usage = widget.get_usage_color("background-color");
        // TODO add style for 'hover' state
        if (focus::is_hot(widget.id)) {
            color_usage = ui::theme::Usage::Accent;
        }
        ui_context->draw_widget_rect(rect, color_usage);
    }

    const auto _press_logic = [&]() -> bool {
        if (focus::matches(widget.id) &&
            ui_context->pressed(InputName::WidgetPress)) {
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

}  // namespace elements
