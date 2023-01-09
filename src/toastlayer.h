
#pragma once

#include "external_include.h"
//
#include "engine/layer.h"
#include "engine/ui_theme.h"
//
#include "toastmanager.h"

struct ToastLayer : public Layer {
    ToastLayer() : Layer("Toasts") {}

    const float toastPctHeight = 0.10f;
    const float toastPctWidth = 0.10f;
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
        for (auto& toast : TOASTS) {
            Color primary = ui::DEFAULT_THEME.from_usage(ui::theme::Primary);
            Color accent = ui::DEFAULT_THEME.from_usage(ui::theme::Accent);
            // TODO interpolate the alpha on these to look nicer, (pop in / pop
            // out)

            DrawRectangleRounded(
                Rectangle{
                    WIN_WF() * 0.8f,           //
                    (WIN_HF() * 0.1f) + offY,  //
                    toastWidth,                //
                    toastHeight,               //
                },
                0.15f, 4, primary);

            DrawRectangleRounded(
                Rectangle{
                    WIN_WF() * 0.8f,                                    //
                    ((WIN_HF() * 0.1f) + offY) + (toastHeight * 0.9f),  //
                    toastWidth * toast.pctOpen,                         //
                    toastHeight * 0.1f,                                 //
                },
                0.15f, 4, accent);

            offY += (bot_padd + toastHeight);
        }
    }
};
