
#pragma once

#include "../globals_register.h"

// TODO again this is not a real .h file, this file just gets pasted in ui.h

namespace ui {

namespace internal {

inline bool should_exit_early(const Widget& widget) {
    if (!context->scissor_box.has_value()) return false;
    return !raylib::CheckCollisionRecs(widget.rect,
                                       context->scissor_box.value());
}

inline ui::UITheme active_theme() { return ui::UI_THEME; }

inline void draw_colored_text(const std::string& content, Rectangle parent,
                              int z_index, Color color = WHITE) {
    callback_registry.register_call(
        context,
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

            // Disable this warning when we are in debug mode since dev facing
            // UI is okay to be too small at the moment

            if (!GLOBALS.get<bool>("debug_ui_enabled")) {
                //  For accessibility reasons, we want to make sure we are
                //  drawing text thats larger than 28px @ 1080p
                float pct_1080 = (WIN_HF() / 1080.f);
                float min_text_size_px = 28;
                if (font_size < (min_text_size_px * pct_1080)) {
                    log_warn(
                        "Rendering text at {} px which is below our min size "
                        "for "
                        "this resolution {}, text was {}",
                        font_size, (min_text_size_px * pct_1080),
                        content.c_str());
                }
            }

            DrawTextEx(font,                //
                       content.c_str(),     //
                       {rect.x, rect.y},    //
                       font_size, spacing,  //
                       color);
        },
        z_index);
}

inline void draw_text(const std::string& content, Rectangle parent, int z_index,
                      ui::theme::Usage color_usage = ui::theme::Usage::Font) {
    draw_colored_text(content, parent, z_index,
                      active_theme().from_usage(color_usage));
}

inline void draw_rect_color(Rectangle rect, int z_index, Color c) {
    callback_registry.register_call(
        context, [=]() { DrawRectangleRounded(rect, 0.25f, 4, c); }, z_index);
}

inline void draw_rect(
    Rectangle rect, int z_index,
    ui::theme::Usage color_usage = ui::theme::Usage::Primary) {
    draw_rect_color(rect, z_index, active_theme().from_usage(color_usage));
}

inline void draw_image(vec2 pos, raylib::Texture texture, float scale,
                       int z_index) {
    callback_registry.register_call(
        context,
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

}  // namespace ui