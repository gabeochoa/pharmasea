
#pragma once

#include "external_include.h"
//
#include "ui_color.h"

namespace ui {

namespace theme {
enum Usage {
    Font,
    Background,
    Primary,
    Secondary,
    Accent,
};
}

struct UITheme {
    Color font;
    Color background;

    Color primary;
    Color secondary;
    Color accent;

    UITheme()
        : font(color::isabelline),
          background(color::oxford_blue),
          primary(color::pacific_blue),
          secondary(color::tea_green),
          accent(color::orange_soda) {}

    UITheme(Color f, Color bg, Color p, Color s, Color a)
        : font(f), background(bg), primary(p), secondary(s), accent(a) {}

    Color from_usage(theme::Usage cu) {
        switch (cu) {
            case theme::Usage::Font:
                return font;
            case theme::Usage::Background:
                return background;
            case theme::Usage::Primary:
                return primary;
            case theme::Usage::Secondary:
                return secondary;
            case theme::Usage::Accent:
                return accent;
        }
    }
};

static const UITheme DEFAULT_THEME = UITheme();
static const UITheme GRAYSCALE =
    UITheme(color::white, color::grey, color::black, color::cool_grey,
            color::off_white);

}  // namespace ui
