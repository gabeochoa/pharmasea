
#pragma once

#include <bitset>

enum Ingredient {
    Invalid = -1,
    Soda,
    Rum,
};

const int MAX_INGREDIENT_TYPES = 32;
using IngredientBitSet = std::bitset<MAX_INGREDIENT_TYPES>;

namespace recipe {

const IngredientBitSet COKE = IngredientBitSet().set(Soda);
const IngredientBitSet RUM_AND_COKE = COKE | IngredientBitSet().set(Rum);

}  // namespace recipe
