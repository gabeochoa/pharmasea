
#pragma once

#include "../engine.h"
#include "../engine/layer.h"
#include "../engine/settings.h"
#include "../preload.h"

struct StreamerSafeLayer : public Layer {
    int streamer_box_size;
    int streamer_box_padd;

    StreamerSafeLayer() : Layer("StreamerSafe") {
        streamer_box_size = WIN_H() / 3;
        streamer_box_padd = 50;
    }
    virtual ~StreamerSafeLayer() {}
    virtual void onUpdate(float) override {}

    virtual void onDraw(float) override {
        // Dont draw on the main menu
        if (MenuState::get().is(menu::State::Root)) return;
        // TODO we probably also want to hide it when doing key binding
        // and maybe in network layer too

        if (Settings::get().data.show_streamer_safe_box) {
            int x_pos = WIN_W() - streamer_box_size - streamer_box_padd;
            int y_pos = WIN_H() - streamer_box_size - streamer_box_padd;
            float FS = 20.f;

            DrawRectangleRoundedLines(
                Rectangle{(float) x_pos, (float) y_pos,
                          (float) streamer_box_size, (float) streamer_box_size},
                0.25f /* roundness */, (int) (FS / 4.f) /* segments */,
                2.0f /* lineThick */, BLACK);

            DrawTextEx(
                Preload::get().font,
                translation_lookup(TranslatableString(strings::i18n::SAFE_ZONE))
                    .c_str(),
                vec2{(float) x_pos + (streamer_box_size / 2.f) - (FS),
                     (float) y_pos + (streamer_box_size / 2.f) - FS},
                FS, 0, WHITE);
        }
    }
};
