
#pragma once

#include "external_include.h"
#include "ui_color.h"

namespace ui {

struct WidgetConfig {
    vec2 position;
    vec2 size;
    float rotation;
    std::string text;
    WidgetConfig* child;
    bool vertical;

    struct Theme {
        enum ColorType {
            FONT = 0,
            FG = 0,
            BG,
        };

        Theme(){}
        Theme(Color font, Color bg)
            : fontColor(font), backgroundColor(bg){}
        Theme(Color font, Color bg, std::string tex)
            : fontColor(font), backgroundColor(bg), texture(tex) {}

        Color fontColor = RAYWHITE;
        Color backgroundColor = color::black;
        std::string texture = "TEXTURE";

        Color color(Theme::ColorType type = ColorType::BG) const {
            if (type == ColorType::FONT || type == ColorType::FG) {
                // TODO raylib color doesnt support negatives ...
                if (fontColor.r < 0)  // default
                    return color::black;
                return fontColor;
            }
            if (backgroundColor.r < 0)  // default
                return color::white;
            return backgroundColor;
        }
    } theme;
};

typedef WidgetConfig::Theme WidgetTheme;

}  // namespace ui
