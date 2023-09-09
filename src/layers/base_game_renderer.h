
#pragma once

#include "../engine/layer.h"
#include "../external_include.h"
#include "raylib.h"
//
#include "../engine.h"
#include "../engine/ui/ui.h"

struct BaseGameRendererLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;

    BaseGameRendererLayer(const std::string& name)
        : Layer(name.c_str()), ui_context(std::make_shared<ui::UIContext>()) {}
    virtual ~BaseGameRendererLayer() {}

    virtual void onUpdate(float) override {}

    virtual void onDrawUI(float dt) = 0;

    bool baseShouldRender() {
        if (MenuState::get().is_not(menu::State::Game)) return false;
        if (GameState::get().is(game::State::Paused)) return false;
        if (shouldSkipRender()) return false;
        return true;
    }

    virtual bool shouldSkipRender() = 0;

    virtual void onDraw(float dt) override {
        if (!baseShouldRender()) return;
        using namespace ui;
        begin(ui_context, dt);
        onDrawUI(dt);
        end();
    }
};
