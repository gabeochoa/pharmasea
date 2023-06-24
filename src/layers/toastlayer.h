
#pragma once

#include "../engine.h"
#include "../engine/toastmanager.h"
#include "../external_include.h"

struct ToastLayer : public Layer, public FontSizeCache {
    ToastLayer() : Layer("Toasts") { FontSizeCache::init(); }

    const float toastPctHeight = 0.05f;
    const float toastPctWidth = 0.25f;
    const float toastPctBottomPadding = 0.01f;

    virtual void onUpdate(float dt) override { toasts::update(dt); }

    virtual void onDraw(float) override {
        // NOTE: no clear since we are toast
        //
        if (TOASTS.empty()) return;

        const int bot_padd = (int) (WIN_HF() * toastPctBottomPadding);
        const float toastWidth = (WIN_WF() * toastPctWidth);
        const float toastHeight = (WIN_HF() * toastPctHeight);
        float offY = 0.f;
        float spacing = 0.f;

        Color accent = ui::DEFAULT_THEME.from_usage(ui::theme::Accent);
        Color font_color = ui::DEFAULT_THEME.from_usage(ui::theme::Font);

        const auto background_color = [](AnnouncementType type) {
            switch (type) {
                case AnnouncementType::Warning:
                    return ui::theme::Secondary;
                case AnnouncementType::Error:
                    return ui::theme::Error;
                default:
                case AnnouncementType::Message:
                    return ui::theme::Primary;
            }
        };

        for (auto& toast : TOASTS) {
            Color primary =
                ui::DEFAULT_THEME.from_usage(background_color(toast.type));
            // TODO interpolate the alpha on these to look nicer, (pop in /
            // pop out)

            vec2 pos = {
                WIN_WF() * 0.72f,           //
                (WIN_HF() * 0.05f) + offY,  //
            };

            DrawRectangleRounded(
                Rectangle{
                    pos.x,        //
                    pos.y,        //
                    toastWidth,   //
                    toastHeight,  //
                },
                0.15f, 4, primary);

            DrawRectangleRounded(
                Rectangle{
                    pos.x,                         //
                    pos.y + (toastHeight * 0.9f),  //
                    toastWidth * toast.pctOpen,    //
                    toastHeight * 0.1f,            //
                },
                0.15f, 4, accent);

            float font_size = get_font_size(std::string(toast.msg), toastWidth,
                                            toastHeight, spacing);
            // TODO we really would prefer to wrap the text, but its kinda
            // hard to do
            font_size = fmaxf(20.f, font_size);

            // TODO at some point we want to split the text onto different
            // lines...
            // https://github.com/emilk/emilib/blob/master/emilib/word_wrap.cpp
            DrawTextEx(font, std::string(toast.msg).c_str(), pos, font_size,
                       spacing, font_color);

            offY += (bot_padd + toastHeight);
        }
    }
};
