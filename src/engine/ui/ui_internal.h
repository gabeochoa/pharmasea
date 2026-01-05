
#pragma once

#include "callback_registry.h"
#include "focus.h"
#include "rect.h"
#include "theme.h"
#include "ui_context.h"
#include "widget.h"
#include "../runtime_globals.h"

namespace ui {

extern UITheme UI_THEME;
extern CallbackRegistry callback_registry;

namespace internal {

inline bool should_exit_early(const Widget& widget) {
    if (!context->scissor_box.has_value()) return false;
    return !raylib::CheckCollisionRecs(widget.rect,
                                       context->scissor_box.value());
}

inline ui::UITheme active_theme() { return ui::UI_THEME; }

inline void draw_colored_text(const TranslatableString& content,
                              Rectangle parent, int z_index,
                              Color color = WHITE) {
    std::string printable_string = translation_lookup(content);

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
            auto font_size = context->get_font_size(
                printable_string, rect.width, rect.height, spacing);

            // Disable this warning when we are in debug mode since dev facing
            // UI is okay to be too small at the moment

            if (!globals::debug_ui_enabled()) {
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
                        printable_string);
                }
            }
            DrawTextEx(font,                      //
                       printable_string.c_str(),  //
                       {rect.x, rect.y},          //
                       font_size, spacing,        //
                       color);
        },
        z_index);
}

inline void draw_text(const TranslatableString& content, Rectangle parent,
                      int z_index,
                      ui::theme::Usage color_usage = ui::theme::Usage::Font) {
    draw_colored_text(content, parent, z_index,
                      active_theme().from_usage(color_usage));
}

struct RectRenderInfo {
    struct RoundedInfo {
        bool rounded = false;
        float roundness = 0.25f;
        int segments = 4;
    } roundedInfo;

    struct OutlineInfo {
        float thickness = 5.f;
        bool outlineOnly = false;
    } outlineInfo;

    // non defaulted params

    Rectangle rect;
    int z_index;
    Color color;
};

inline void draw_any_rect(const RectRenderInfo& info) {
    callback_registry.register_call(
        context,
        [=]() {
            if (info.roundedInfo.rounded) {
                if (info.outlineInfo.outlineOnly) {
                    // Raylib 5.5: DrawRectangleRoundedLines no longer takes
                    // line thickness
                    DrawRectangleRoundedLines(
                        info.rect, info.roundedInfo.roundness,
                        info.roundedInfo.segments, info.color);
                } else {
                    DrawRectangleRounded(info.rect, info.roundedInfo.roundness,
                                         info.roundedInfo.segments, info.color);
                }
            } else {
                if (info.outlineInfo.outlineOnly) {
                    DrawRectangleLinesEx(info.rect, info.outlineInfo.thickness,
                                         info.color);
                } else {
                    DrawRectangleRec(info.rect, info.color);
                }
            }
        },
        info.z_index);
}

inline void draw_rect_color(Rectangle rect, int z_index, Color color,
                            bool rounded = false, bool outlineOnly = false) {
    draw_any_rect(RectRenderInfo{
        .roundedInfo =
            RectRenderInfo::RoundedInfo{
                .rounded = rounded,
            },
        .outlineInfo =
            RectRenderInfo::OutlineInfo{
                .outlineOnly = outlineOnly,
            },
        //
        .rect = rect,
        .z_index = z_index,
        .color = color,
    });
}

inline void draw_rect(Rectangle rect, int z_index,
                      ui::theme::Usage color_usage = ui::theme::Usage::Primary,
                      bool rounded = false, bool outlineOnly = false) {
    draw_rect_color(rect, z_index, active_theme().from_usage(color_usage),
                    rounded, outlineOnly);
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

inline void draw_focus_ring(const Widget& widget, bool rounded = false) {
    if (!focus::matches(widget.id)) return;
    Rectangle rect = widget.get_rect();
    float pixels = WIN_HF() * 0.003f;
    rect = rect::expand(rect, {pixels, pixels, pixels, pixels});
    internal::draw_rect(rect, widget.z_index, ui::theme::Usage::Accent, rounded,
                        true /* outline only */);
}

}  // namespace internal
}  // namespace ui
