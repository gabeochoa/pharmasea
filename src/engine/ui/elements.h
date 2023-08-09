

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

inline bool is_mouse_inside(const MouseInfo& mouse_info,
                            const Rectangle& rect) {
    auto mouse = mouse_info.pos;
    return mouse.x >= rect.x && mouse.x <= rect.x + rect.width &&
           mouse.y >= rect.y && mouse.y <= rect.y + rect.height;
}

inline bool is_mouse_down_in_box(const Rectangle& rect) {
    auto minfo = get_mouse_info();
    return is_mouse_inside(minfo, rect) && minfo.leftDown;
}

struct Widget {
    ui::uuid id;
    LayoutBox layout_box;

    Widget(const LayoutBox& lb) : layout_box(lb) {}

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

    auto image = widget.get_possible_background_image();
    if (image.has_value()) {
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
        ui_context->draw_widget_rect(rect, color_usage);
    }

    if (is_mouse_down_in_box(rect)) {
        if (!widget.layout_box.node.attrs.contains("id")) {
            log_warn("you have a button without an id {} {}",
                     widget.layout_box.node.tag,
                     widget.layout_box.node.children[0].content);
            return false;
        }
        return true;
    }
    return false;
}

}  // namespace elements
