
#include "recipe_book_layer.h"

#include "../ah.h"
#include "../components/is_progression_manager.h"
#include "../dataclass/ingredient.h"
#include "../engine/input_utilities.h"
#include "../entity_helper.h"
#include "../entity_query.h"
#include "../libraries/recipe_library.h"
#include "base_game_renderer.h"

OptEntity get_ipm_entity() {
    return EntityQuery().whereHasComponent<IsProgressionManager>().gen_first();
}

int num_recipes() {
    auto ent = get_ipm_entity();
    if (!ent) {
        log_info("no num ent");
        return 0;
    }
    const IsProgressionManager& ipm = ent->get<IsProgressionManager>();
    return (int) ipm.enabled_drinks().count();
}

Drink get_drink_for_selected_id(int selected_recipe) {
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

void RecipeBookLayer::onDrawUI(float) {
    using namespace ui;

    const auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
    auto content = rect::tpad(window, 50);
    content = rect::lpad(content, 0);
    content = rect::rpad(content, 30);

    auto left = rect::rpad(content, 30);
    auto right = rect::lpad(content, 30);

    left = rect::tpad(left, 10);
    left = rect::lpad(left, 10);
    left = rect::bpad(left, 90);
    left = rect::rpad(left, 90);

    right = rect::tpad(right, 10);
    right = rect::rpad(right, 90);
    right = rect::bpad(right, 90);

    Drink drink = get_drink_for_selected_id(selected_recipe);

    div(Widget{content}, ui::theme::Background);

    image(Widget{left}, get_icon_name_for_drink(drink));

    div(Widget{right}, ui::theme::Secondary);

    auto title = rect::bpad(right, 20);
    title = rect::lpad(title, 5);

    auto description = rect::tpad(right, 20);
    description = rect::lpad(description, 10);

    text(Widget{title}, TODO_TRANSLATE(get_string_for_drink(drink),
                                       TodoReason::SubjectToChange));

    // TODO we dont have a static max ingredients
    const auto igs = rect::hsplit<MAX_VISIBLE_IGS>(description);

    auto ingredients = get_recipe_for_drink(drink);
    int i = 0;

    bitset_utils::for_each_enabled_bit(ingredients, [&](size_t bit) {
        if (i > MAX_VISIBLE_IGS) return bitset_utils::ForEachFlow::Break;
        Ingredient ig = magic_enum::enum_value<Ingredient>(bit);
        text(Widget{igs[i]}, TODO_TRANSLATE(get_string_for_ingredient(ig),
                                            TodoReason::SubjectToChange));
        i++;
        return bitset_utils::ForEachFlow::NormalFlow;
    });

    auto index = rect::tpad(content, 80);
    index = rect::lpad(index, 10);
    index = rect::rpad(index, 20);

    text(Widget{index}, NO_TRANSLATE(fmt::format("{:2}/{}", selected_recipe + 1,
                                                 num_recipes())));
}
