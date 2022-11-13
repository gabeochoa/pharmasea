
#pragma once

#include "app.h"
#include "external_include.h"
#include "layer.h"
#include "menu.h"

struct MenuStateLayer : public Layer {
    MenuStateLayer() : Layer("MenuState") { minimized = false; }
    virtual ~MenuStateLayer() {}
    virtual void onEvent(Event&) override {}
    virtual void onUpdate(float) override {}

    virtual void onDraw(float) override {
        DrawTextEx(Preload::get().font, Menu::get().tostring(), {5, 20}, 20, 0,
                   WHITE);
    }
};
