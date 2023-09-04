
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
        if (Settings::get().data.show_streamer_safe_box) {
            int x_pos = WIN_W() - streamer_box_size - streamer_box_padd;
            int y_pos = WIN_H() - streamer_box_size - streamer_box_padd;
            float FS = 20.f;

            DrawRectangleRoundedLines(
                Rectangle{(float) x_pos, (float) y_pos,
                          (float) streamer_box_size, (float) streamer_box_size},
                0.25f /* roundness */, (int) (FS / 4.f) /* segments */,
                (FS / 4.f) /* lineThick */, BLACK);

            DrawTextEx(Preload::get().font,
                       text_lookup(strings::i18n::SAFE_ZONE),
                       vec2{(float) x_pos + (streamer_box_size / 2.f) - (FS),
                            (float) y_pos + (streamer_box_size / 2.f) - FS},
                       FS, 0, WHITE);
        }
    }
};
