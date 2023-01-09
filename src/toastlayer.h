
#pragma once

#include "external_include.h"
//
#include "engine/font_sizer.h"
#include "engine/layer.h"
#include "engine/ui_theme.h"
//
#include "toastmanager.h"

struct ToastLayer : public Layer, public FontSizeCache {
    ToastLayer() : Layer("Toasts") { FontSizeCache::init(); }

    const float toastPctHeight = 0.05f;
    const float toastPctWidth = 0.25f;
    const float toastPctBottomPadding = 0.01f;

    virtual void onEvent(Event&) override {}
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

        for (auto& toast : TOASTS) {
            Color primary = ui::DEFAULT_THEME.from_usage(ui::theme::Primary);
            Color accent = ui::DEFAULT_THEME.from_usage(ui::theme::Accent);
            Color font_color = ui::DEFAULT_THEME.from_usage(ui::theme::Font);
            // TODO interpolate the alpha on these to look nicer, (pop in / pop
            // out)

            vec2 pos = {
                WIN_WF() * 0.72f,          //
                (WIN_HF() * 0.1f) + offY,  //
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
            DrawTextEx(font, std::string(toast.msg).c_str(), pos, font_size,
                       spacing, font_color);

            offY += (bot_padd + toastHeight);
        }
    }
};
