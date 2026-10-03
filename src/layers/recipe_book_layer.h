#pragma once

#include "../ah.h"
#include "../engine/input_helper.h"
#include "../engine/input_utilities.h"
#include "../engine/keymap.h"
#include "base_game_renderer.h"

constexpr int MAX_VISIBLE_IGS = 10;

struct RecipeBookLayer : public BaseGameRendererLayer {
    bool should_show_recipes = false;

    float selected_recipe_debounce = 0.f;
    float selected_recipe_debounce_reset = 0.100f;

    int selected_recipe = 0;

    RecipeBookLayer() : BaseGameRendererLayer("RecipeBook") {}

    virtual ~RecipeBookLayer() {}

    virtual bool shouldSkipRender() override { return !shouldRender(); }
    bool shouldRender() { return should_show_recipes; }

    void handleInput() {
        if (GameState::get().is_not(game::State::InGame)) return;

        // Polling-based recipe book toggle (replaces
        // onKeyPressed/onGamepadButtonPressed handlers)

        // Close with Pause when showing - consume to prevent GameLayer from
        // also pausing
        if (should_show_recipes &&
            input_helper::was_pressed(InputName::Pause)) {
            input_helper::consume_pressed(InputName::Pause);
            should_show_recipes = false;
            return;
        }

        // Toggle recipe book
        if (input_helper::was_pressed(InputName::ShowRecipeBook)) {
            input_helper::consume_pressed(InputName::ShowRecipeBook);
            should_show_recipes = !should_show_recipes;
            return;
        }

        // Recipe navigation when showing
        if (!baseShouldRender()) return;

        // Navigate left
        if (selected_recipe_debounce <= 0 &&
            input_helper::was_pressed(InputName::ValueLeft)) {
            extern int num_recipes();
            if (num_recipes() > 0) {
                selected_recipe = (int) fmax(0, selected_recipe - 1);
                selected_recipe_debounce = selected_recipe_debounce_reset;
            }
        }

        // Navigate right / next
        if (selected_recipe_debounce <= 0) {
            bool right_pressed =
                input_helper::was_pressed(InputName::ValueRight);
            bool next_pressed =
                input_helper::was_pressed(InputName::RecipeNext);

            if (right_pressed || next_pressed) {
                extern int num_recipes();
                int nr = num_recipes();
                if (nr > 0) {
                    selected_recipe = (int) fmin(nr - 1, selected_recipe + 1);
                    selected_recipe_debounce = selected_recipe_debounce_reset;
                }
            }
        }
    }

    virtual void onUpdate(float dt) override {
        if (selected_recipe_debounce > 0) selected_recipe_debounce -= dt;

        handleInput();
    }

    virtual void onDrawUI(float) override;
};
