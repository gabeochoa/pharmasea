
#pragma once

#include "external_include.h"
#include "layer.h"
#include "menu.h"
#include "app.h"

struct MenuStateLayer : public Layer {
    MenuStateLayer() : Layer("MenuState") { minimized = false; }
    virtual ~MenuStateLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}
    virtual void onEvent(Event&) override {}
    virtual void onUpdate(float) override {}

    virtual void onDraw(float) override {
        // TODO with gamelayer, support events
        if (minimized) {
            return;
        }
        DrawTextEx(
                App::get().font, 
                Menu::get().tostring(), 
                {5, 20},
                20, 0,
                LIGHTGRAY);
    }
};
