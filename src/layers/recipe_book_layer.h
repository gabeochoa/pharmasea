#pragma once

#include "base_game_renderer.h"

constexpr int MAX_VISIBLE_IGS = 10;

struct RecipeBookLayer : public BaseGameRendererLayer {
    bool should_show_recipes = false;

    float selected_recipe_debounce = 0.f;
    float selected_recipe_debounce_reset = 0.100f;

    int selected_recipe = 0;

    RecipeBookLayer() : BaseGameRendererLayer("RecipeBook") {}

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) override;
    bool onKeyPressed(KeyPressedEvent& event) override;
    virtual ~RecipeBookLayer() {}

    virtual bool shouldSkipRender() override { return !shouldRender(); }
    bool shouldRender() { return should_show_recipes; }

    virtual void onUpdate(float dt) override {
        if (selected_recipe_debounce > 0) selected_recipe_debounce -= dt;
    }

    virtual void onDrawUI(float) override;
};
