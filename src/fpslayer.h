#pragma once

#include "external_include.h"
#include "layer.h"

struct FPSLayer : public Layer {
    FPSLayer() : Layer("FPS") { minimized = false; }
    virtual ~FPSLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}
    virtual void onEvent(Event&) override {}
    virtual void onUpdate(float) override {}

    virtual void onDraw(float) override {
        // TODO with gamelayer, support events
        if (minimized) {
            return;
        }
        raylib::DrawFPS(0, 0);
    }
};
