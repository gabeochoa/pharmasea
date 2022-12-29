#pragma once

#include "external_include.h"
#include "globals.h"
#include "layer.h"
#include "preload.h"

struct VersionLayer : public Layer {
    VersionLayer() : Layer("Version") {}
    virtual void onEvent(Event&) override {}
    virtual void onUpdate(float) override {}

    virtual void onDraw(float) override {
        DrawTextEx(Preload::get().font, VERSION.data(), {5, 30}, 20, 0, WHITE);
    }
};
