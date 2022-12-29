
#pragma once

#include "external_include.h"
#include "layer.h"
#include "menu.h"
#include "preload.h"

struct MenuStateLayer : public Layer {
    MenuStateLayer() : Layer("MenuState") {}
    virtual ~MenuStateLayer() {}
    virtual void onEvent(Event&) override {}
    virtual void onUpdate(float) override {}

    virtual void onDraw(float) override {
        DrawTextEx(Preload::get().font, Menu::get().tostring(), {5, 20}, 20, 0,
                   WHITE);
    }
};
