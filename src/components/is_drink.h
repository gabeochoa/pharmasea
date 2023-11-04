

#pragma once

#include <bitset>

#include "../dataclass/ingredient.h"
#include "../recipe_library.h"
#include "../vendor_include.h"
#include "base_component.h"

struct IsDrink : public BaseComponent {
    virtual ~IsDrink() {}

    [[nodiscard]] bool has_ingredient(Ingredient i) const {
        int index = magic_enum::enum_integer<Ingredient>(i);
        return ingredients[index];
    }

    void add_ingredient(Ingredient i) {
        log_info("added ingredient {}", magic_enum::enum_name<Ingredient>(i));
        int index = magic_enum::enum_integer<Ingredient>(i);
        ingredients[index] = true;

        // Also run algo to check what drink this makes
        underlying = calc_underlying();
    }

    [[nodiscard]] std::optional<Drink> calc_underlying() {
        const auto recipelibrary = RecipeLibrary::get();

        for (const auto& recipe : recipelibrary) {
            if (matches_recipe(recipe.second.drink)) {
                return recipe.second.drink;
            }
        }

        return {};
    }

    [[nodiscard]] bool matches_recipe(const IngredientBitSet& recipe) const {
        bool has_req = (recipe & ingredients) == recipe;
        bool has_extra = (recipe ^ ingredients).any();
        return has_req && !has_extra;
    }

    [[nodiscard]] bool matches_recipe(const Drink& drink_name) const {
        return matches_recipe(
            RecipeLibrary::get()
                .get(std::string(magic_enum::enum_name(drink_name)))
                .ingredients);
    }

    [[nodiscard]] const IngredientBitSet ing() const { return ingredients; }

    std::optional<Drink> underlying;

   private:
    IngredientBitSet ingredients;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.ext(ingredients, bitsery::ext::StdBitset{});

        s.ext(underlying, bitsery::ext::StdOptional{},
              [](S& sv, Drink& val) { sv.value4b(val); });
    }
};
