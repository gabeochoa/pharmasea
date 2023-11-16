
#pragma once

#include <bitset>

#include "../dataclass/ingredient.h"
#include "../recipe_library.h"
#include "../vendor_include.h"
#include "base_component.h"

using StdMap = bitsery::ext::StdMap;

struct IsDrink : public BaseComponent {
    IsDrink() : supports_multiple(false) {}
    virtual ~IsDrink() {}

    auto& turn_on_support_multiple() {
        supports_multiple = true;
        return *this;
    }

    [[nodiscard]] bool can_add(Ingredient i) const {
        int count = count_of_ingredient(i);
        if (count == 0) return true;
        if (count >= 1 && supports_multiple) return true;
        return false;
    }

    [[nodiscard]] bool has_ingredient(Ingredient i) const {
        return ingredients.contains(i) && ingredients.at(i) > 0;
    }

    void add_ingredient(Ingredient i) {
        ingredients[i]++;
        unique_igs[magic_enum::enum_integer<Ingredient>(i)] = true;

        // Also run algo to check what drink this makes
        underlying = calc_underlying();

        num_completed = calc_completed();

        log_info("added ingredient {} ({}) to multi. matching {} {} times",
                 magic_enum::enum_name<Ingredient>(i), count_of_ingredient(i),
                 underlying.has_value(), num_completed);
    }

    [[nodiscard]] bool matches_recipe(const IngredientBitSet& recipe) const {
        bool has_req = (recipe & unique_igs) == recipe;
        bool has_extra = (recipe ^ unique_igs).any();
        return has_req && !has_extra;
    }

    [[nodiscard]] bool matches_recipe(const Drink& drink_name) const {
        return matches_recipe(
            RecipeLibrary::get()
                .get(std::string(magic_enum::enum_name(drink_name)))
                .ingredients);
    }

    [[nodiscard]] IngredientBitSet ing() const { return unique_igs; }

    [[nodiscard]] int count_of_ingredient(Ingredient ig) const {
        return ingredients.contains(ig) ? ingredients.at(ig) : 0;
    }

    [[nodiscard]] int get_num_complete() const { return num_completed; }
    [[nodiscard]] bool has_anything() const { return unique_igs.any(); }

    void remove_one_completed() {
        if (!underlying.has_value()) return;
        IngredientBitSet recipe = get_recipe_for_drink(underlying.value());
        bitset_utils::for_each_enabled_bit(recipe, [&](size_t bit) {
            Ingredient ig = magic_enum::enum_value<Ingredient>(bit);
            ingredients[ig]--;
        });

        underlying = calc_underlying();
        num_completed = calc_completed();
        if (num_completed == 0) {
            unique_igs.reset();
        }
    }

    std::optional<Drink> underlying;

   private:
    [[nodiscard]] int calc_completed() {
        // If theres no matching drink then nothing in here yet
        if (!underlying.has_value()) return 0;

        IngredientBitSet recipe = get_recipe_for_drink(underlying.value());
        // if we have a matching recipe then we should have already one set
        int min_igs = 99;
        bitset_utils::for_each_enabled_bit(recipe, [&](size_t bit) {
            Ingredient ig = magic_enum::enum_value<Ingredient>(bit);
            min_igs = std::min(min_igs, count_of_ingredient(ig));
        });
        return min_igs;
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

    int num_completed = 0;
    std::map<Ingredient, int> ingredients;
    IngredientBitSet unique_igs;
    bool supports_multiple = false;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value1b(supports_multiple);

        s.value4b(num_completed);

        s.ext(ingredients, StdMap{magic_enum::enum_count<Ingredient>()},
              [](S& sv, Ingredient& key, int value) {
                  sv.value4b(key);
                  sv.value4b(value);
              });

        s.ext(unique_igs, bitsery::ext::StdBitset{});

        s.ext(underlying, bitsery::ext::StdOptional{},
              [](S& sv, Drink& val) { sv.value4b(val); });
    }
};
