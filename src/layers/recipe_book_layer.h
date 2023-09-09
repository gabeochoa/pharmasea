#pragma once

#include "../components/is_progression_manager.h"
#include "../dataclass/ingredient.h"
#include "../engine/bitset_utils.h"
#include "../entity_helper.h"
#include "../recipe_library.h"
#include "base_game_renderer.h"

const int MAX_VISIBLE_IGS = 10;

struct RecipeBookLayer : public BaseGameRendererLayer {
    bool should_show_recipes = false;

    float selected_recipe_debounce = 0.f;
    float selected_recipe_debounce_reset = 0.100f;

    int selected_recipe = 0;

    int num_recipes() {
        auto ent = get_ipm_entity();
        if (!ent) {
            log_info("no num ent");
            return 0;
        }
        const IsProgressionManager& ipm = ent->get<IsProgressionManager>();
        return (int) ipm.enabled_drinks().count();
    }

    RecipeBookLayer() : BaseGameRendererLayer("RecipeBook") {}

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) override {
        if (KeyMap::get_button(menu::State::Game, InputName::ShowRecipeBook) ==
            event.button) {
            should_show_recipes = !should_show_recipes;
            return true;
        }
        // TODO add game buttons for this
        if (!baseShouldRender()) return false;

        if (selected_recipe_debounce <= 0 &&
            KeyMap::get_button(menu::State::UI, InputName::ValueLeft) ==
                event.button) {
            selected_recipe = (int) fmax(0, selected_recipe - 1);
            selected_recipe_debounce = selected_recipe_debounce_reset;
            return true;
        }
        if (selected_recipe_debounce <= 0 &&
            KeyMap::get_button(menu::State::UI, InputName::ValueRight) ==
                event.button) {
            selected_recipe =
                (int) fmin(num_recipes() - 1, selected_recipe + 1);
            selected_recipe_debounce = selected_recipe_debounce_reset;
            return true;
        }

        return ui_context->process_gamepad_button_event(event);
    }

    bool onKeyPressed(KeyPressedEvent& event) override {
        if (GameState::get().is_not(game::State::InRound) &&
            GameState::get().is_not(game::State::Planning))
            return false;

        if (should_show_recipes &&
            KeyMap::get_key_code(menu::State::Game, InputName::Pause) ==
                event.keycode) {
            should_show_recipes = false;
            return true;
        }

        if (KeyMap::get_key_code(menu::State::Game,
                                 InputName::ShowRecipeBook) == event.keycode) {
            should_show_recipes = !should_show_recipes;
            return true;
        }

        if (!baseShouldRender()) return false;

        if (selected_recipe_debounce <= 0 &&
            KeyMap::get_key_code(menu::State::UI, InputName::ValueLeft) ==
                event.keycode) {
            selected_recipe = (int) fmax(0, selected_recipe - 1);
            selected_recipe_debounce = selected_recipe_debounce_reset;
            return true;
        }
        if (selected_recipe_debounce <= 0 &&
            KeyMap::get_key_code(menu::State::UI, InputName::ValueRight) ==
                event.keycode) {
            selected_recipe =
                (int) fmin(num_recipes() - 1, selected_recipe + 1);
            selected_recipe_debounce = selected_recipe_debounce_reset;
            return true;
        }
        return ui_context->process_keyevent(event);
    }

    virtual ~RecipeBookLayer() {}

    virtual bool shouldSkipRender() override { return !shouldRender(); }

    bool shouldRender() { return should_show_recipes; }

    virtual void onUpdate(float dt) override {
        if (selected_recipe_debounce > 0) selected_recipe_debounce -= dt;
    }

    OptEntity get_ipm_entity() {
        return EntityHelper::getFirstWithComponent<IsProgressionManager>();
    }

    Drink get_drink_for_selected_id() {
        auto ent = get_ipm_entity();
        if (!ent) {
            return Drink::coke;
        }
        if (num_recipes() == 0) {
            return Drink::coke;
        }

        const IsProgressionManager& ipm = ent->get<IsProgressionManager>();
        const DrinkSet drinks = ipm.enabled_drinks();

        size_t drink_index =
            bitset_utils::index_of_nth_set_bit(drinks, selected_recipe + 1);

        return magic_enum::enum_value<Drink>(drink_index);
    }

    virtual void onDrawUI(float) override {
        using namespace ui;

        const auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        auto content = rect::tpad(window, 20);
        content = rect::lpad(content, 20);
        content = rect::rpad(content, 75);

        auto [left, right] = rect::vsplit<2>(content, 10);
        left = rect::lpad(left, 20);
        left = rect::tpad(left, 20);
        left = rect::bpad(left, 75);

        right = rect::rpad(right, 80);
        right = rect::tpad(right, 20);
        right = rect::bpad(right, 75);

        Drink drink = get_drink_for_selected_id();

        div(Widget{content}, ui::theme::Background);

        image(Widget{left}, get_icon_name_for_drink(drink));

        div(Widget{right}, ui::theme::Secondary);

        const auto title = rect::bpad(right, 20);
        auto description = rect::tpad(right, 20);
        description = rect::lpad(description, 10);

        text(Widget{title}, fmt::format("{}", get_string_for_drink(drink)));

        // TODO we dont have a static max ingredients
        const auto igs = rect::hsplit<MAX_VISIBLE_IGS>(description);

        auto ingredients = get_recipe_for_drink(drink);
        int i = 0;

        bitset_utils::for_each_enabled_bit(ingredients, [&](size_t bit) {
            if (i > MAX_VISIBLE_IGS) return;
            Ingredient ig = magic_enum::enum_value<Ingredient>(bit);
            text(Widget{igs[i]},
                 fmt::format("{}", get_string_for_ingredient(ig)));
            i++;
        });

        auto index = rect::tpad(content, 90);
        index = rect::lpad(index, 90);

        text(Widget{index},
             fmt::format("{:2}/{}", selected_recipe + 1, num_recipes()));
    }
};
