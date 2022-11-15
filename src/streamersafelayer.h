
#pragma once

#include "layer.h"
#include "settings.h"

constexpr int STREAMER_BOX_SIZE = WIN_H / 3;
constexpr int STREAMER_BOX_PADD = 50;

struct StreamerSafeLayer : public Layer {
    StreamerSafeLayer() : Layer("StreamerSafe") {}
    virtual ~StreamerSafeLayer() {}
    virtual void onEvent(Event&) override {}
    virtual void onUpdate(float) override {}

    virtual void onDraw(float) override {
        if (Settings::get().data.show_streamer_safe_box) {
            int x_pos = WIN_W - STREAMER_BOX_SIZE - STREAMER_BOX_PADD;
            int y_pos = WIN_H - STREAMER_BOX_SIZE - STREAMER_BOX_PADD;
            float FS = 20.f;

            DrawRectangleRoundedLines(
                Rectangle{(float) x_pos, (float) y_pos, STREAMER_BOX_SIZE,
                          STREAMER_BOX_SIZE},
                0.25f /* roundness */, (int) (FS / 4.f) /* segments */,
                (FS / 4.f) /* lineThick */, BLACK);

            DrawTextEx(Preload::get().font, "safe zone",
                       vec2{(float) x_pos + (STREAMER_BOX_SIZE / 2.f) - (FS),
                            (float) y_pos + (STREAMER_BOX_SIZE / 2.f) - FS},
                       FS, 0, WHITE);
        }
    }
};
