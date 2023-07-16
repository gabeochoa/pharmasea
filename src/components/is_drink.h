

#pragma once

#include <bitset>

#include "../dataclass/ingredient.h"
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
    }

    [[nodiscard]] bool matches_recipe(const IngredientBitSet& recipe) const {
        bool has_req = (recipe & ingredients) == recipe;
        bool has_extra = (recipe ^ ingredients).any();
        return has_req && !has_extra;
    }

   private:
    IngredientBitSet ingredients;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.ext(ingredients, bitsery::ext::StdBitset{});
    }
};
