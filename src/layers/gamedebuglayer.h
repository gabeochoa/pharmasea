#pragma once

#include "../engine/layer.h"
#include "../engine/ui/ui.h"

struct GameDebugLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;

    GameDebugLayer() : Layer(strings::menu::GAME) {
        ui_context = std::make_shared<ui::UIContext>();
    }
    virtual ~GameDebugLayer() {}

    virtual void onUpdate(float) override;
    virtual void onDraw(float dt) override;

    void draw_debug_ui(float dt);
};
