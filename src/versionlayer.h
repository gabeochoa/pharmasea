#pragma once

#include "app.h"
#include "external_include.h"
#include "globals.h"
#include "layer.h"
#include "ui_color.h"

struct VersionLayer : public Layer {
    VersionLayer() : Layer("Version") { minimized = false; }
    virtual ~VersionLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}
    virtual void onEvent(Event&) override {}
    virtual void onUpdate(float) override {}

    virtual void onDraw(float) override {
        if (minimized) {
            return;
        }
        raylib::DrawTextEx(Preload::get().font, VERSION.c_str(), {5, 30}, 20, 0,
                           ui::color::light_grey);
    }
};
