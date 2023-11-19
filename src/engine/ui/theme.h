
#pragma once

#include "color.h"

namespace ui {

namespace theme {
enum Usage {
    Font,
    DarkFont,
    Background,
    Primary,
    Secondary,
    Accent,
    Error,
};
}

struct UITheme {
    Color font;
    Color darkfont;
    Color background;

    Color primary;
    Color secondary;
    Color accent;
    Color error;

    UITheme()
        : font(color::isabelline),
          darkfont(color::oxford_blue),
          background(color::oxford_blue),
          primary(color::pacific_blue),
          secondary(color::tea_green),
          accent(color::orange_soda),
          // TODO find a better error color
          error(color::red) {}

    UITheme(Color f, Color df, Color bg, Color p, Color s, Color a, Color e)
        : font(f),
          darkfont(df),
          background(bg),
          primary(p),
          secondary(s),
          accent(a),
          error(e) {}

    Color from_usage(theme::Usage cu) const {
        switch (cu) {
            case theme::Usage::Font:
                return font;
            case theme::Usage::DarkFont:
                return darkfont;
            case theme::Usage::Background:
                return background;
            case theme::Usage::Primary:
                return primary;
            case theme::Usage::Secondary:
                return secondary;
            case theme::Usage::Accent:
                return accent;
            case theme::Usage::Error:
                return error;
        }
        return background;
    }
};

extern UITheme UI_THEME;
static const UITheme GRAYSCALE =
    UITheme(color::white, color::grey, color::grey, color::black,
            color::cool_grey, color::off_white,
            // TODO is there a better error color?
            color::off_white);

[[nodiscard]] inline Color get_progress_bar_colors(float value,  //
                                                   float pct_warning = -1,
                                                   float pct_error = -1) {
    if (pct_warning != -1 && value < pct_error)
        return UI_THEME.from_usage(theme::Usage::Error);
    if (pct_error != -1 && value < pct_warning)
        return UI_THEME.from_usage(theme::Usage::Accent);
    return UI_THEME.from_usage(theme::Usage::Primary);
}

[[nodiscard]] inline Color get_default_progress_bar_color(float value) {
    return get_progress_bar_colors(value, 0.05f, 0.2f);
}

}  // namespace ui
