
#pragma once

#include "../engine.h"
#include "../engine/toastmanager.h"
#include "../engine/ui/context.h"
#include "../external_include.h"

struct ToastLayer : public BaseGameRendererLayer {
    std::shared_ptr<ui::UIContext> ui_context;

    ToastLayer() : BaseGameRendererLayer("Toasts") {}

    virtual void onUpdate(float dt) override { toasts::update(dt); }

    virtual bool shouldSkipRender() override {
        if (TOASTS.empty()) return true;
        return false;
    }

    virtual void onDrawUI(float) override {
        using namespace ui;
        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};

        window = rect::lpad(window, 80);
        window = rect::rpad(window, 80);
        window = rect::tpad(window, 10);
        window = rect::bpad(window, 95);

        const int MX_TOASTS = 10;
        auto toast_spots = rect::hsplit<MX_TOASTS>(window, 10);

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

        // TODO interpolate the alpha on these to look nicer, (pop in /
        // pop out)

        Color accent = UI_THEME.from_usage(ui::theme::Accent);
        int i = MX_TOASTS - 1;
        for (auto& toast : TOASTS) {
            Rectangle spot = toast_spots[i];
            Color primary = UI_THEME.from_usage(background_color(toast.type));
            div(Widget{Rectangle{
                    spot.x,
                    spot.y + (WIN_HF() * 0.005f),
                    spot.width * toast.pctOpen,
                    spot.height,
                }},
                accent);
            div(Widget{spot}, primary);
            text(Widget{spot}, toast.msg);

            i--;
            if (i < 0) break;
        }

        // TODO at some point we want to split the text onto different
        // lines...
        // https://github.com/emilk/emilib/blob/master/emilib/word_wrap.cpp
    }
};
