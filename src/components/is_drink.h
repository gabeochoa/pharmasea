
#pragma once

#include <bitset>

#include "../dataclass/ingredient.h"
#include "../recipe_library.h"
#include "../vendor_include.h"
#include "base_component.h"

using StdMap = bitsery::ext::StdMap;

struct IsDrink : public BaseComponent {
    IsDrink() : _supports_multiple(false) {}
    virtual ~IsDrink() {}

    auto& turn_on_support_multiple(int max) {
        _supports_multiple = true;
        max_count = max;
        return *this;
    }

    [[nodiscard]] bool can_add(Ingredient i) const {
        int count = count_of_ingredient(i);
        if (count == 0) return true;
        if (count >= 1 && _supports_multiple && count <= max_count) return true;
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

        log_info("added ingredient {} ({}) to cup. matching {} {} times",
                 magic_enum::enum_name<Ingredient>(i), count_of_ingredient(i),
                 underlying.has_value(), num_completed);
    }

    [[nodiscard]] bool matches_recipe(const Recipe& recipe,
                                      const IngredientBitSet& igs,
                                      bool ignore_num_drinks = false) const {
        // if this recipe needs multiple (ie pitcher)
        // then wait until its the matching one
        if ((!ignore_num_drinks) && recipe.num_drinks != num_completed)
            return false;

        bool has_req = (igs & unique_igs) == igs;
        bool has_extra = (igs ^ unique_igs).any();
        return has_req && !has_extra;
    }

    [[nodiscard]] bool matches_drink(const Drink& drink_name,
                                     bool ignore_num_drinks = false) const {
        Recipe recipe = RecipeLibrary::get().get(
            std::string(magic_enum::enum_name(drink_name)));
        return matches_recipe(recipe, recipe.ingredients, ignore_num_drinks);
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

    [[nodiscard]] float get_tip_multiplier() const { return tip_multiplier; }

    void fold_tip_multiplier(float amt) { tip_multiplier *= amt; }

    [[nodiscard]] bool supports_multiple() const { return _supports_multiple; }

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
            if (matches_drink(recipe.second.drink, true)) {
                return recipe.second.drink;
            }
        }

        return {};
    }

    int max_count = 1;
    int num_completed = 0;
    std::map<Ingredient, int> ingredients;
    IngredientBitSet unique_igs;
    bool _supports_multiple = false;

    float tip_multiplier = 1.f;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value1b(_supports_multiple);

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
