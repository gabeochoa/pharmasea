
#pragma once

#include "../engine.h"
#include "../external_include.h"
#include "../preload.h"

struct MenuStateLayer : public Layer {
    MenuStateLayer() : Layer("MenuState") {}
    virtual ~MenuStateLayer() {}
    virtual void onUpdate(float) override {}

    // TODO better system for handling this kind of stuff
    // basically version kinda overlaps with this text....
    virtual void onDraw(float) override {
        DrawTextEx(Preload::get().font,
                   std::string(MenuState::get().tostring()).c_str(), {5, 20},
                   20, 0, WHITE);
        DrawTextEx(Preload::get().font,
                   std::string(GameState::get().tostring()).c_str(), {5, 50},
                   20, 0, WHITE);
    }
};
