
#pragma once

#include <bitset>

enum Ingredient {
    Invalid = -1,

    IceCubes,
    IceCrushed,

    // Non Alcoholic
    Soda,
    Water,
    Tonic,

    // Garnishes
    Lime,
    Salt,
    MintLeaf,

    // Liquor
    Rum,
    Tequila,  // Silver or gold?
    Vodka,
    Whiskey,  // Rye or Bourbon?
    Gin,

    // What are these
    TripleSec,
    Cointreau,  // seems like this is a triplesec alt TODO support alts?
    Bitters,

    // Lemon
    Lemon,
    LemonJuice,

    // Juice,
    LimeJuice,
    CranJuice,
    PinaJuice,
    CoconutCream,
    SimpleSyrup,

};

namespace ingredient {
const Ingredient ALC_START = Ingredient::Rum;
const Ingredient ALC_END = Ingredient::Gin;

const Ingredient LEMON_START = Ingredient::Lemon;
const Ingredient LEMON_END = Ingredient::LemonJuice;

}  // namespace ingredient

const int MAX_INGREDIENT_TYPES = 32;
using IngredientBitSet = std::bitset<MAX_INGREDIENT_TYPES>;

namespace recipe {

const IngredientBitSet COKE = IngredientBitSet().set(Soda);
const IngredientBitSet RUM_AND_COKE = COKE | IngredientBitSet().set(Rum);

const IngredientBitSet MARGARITA = IngredientBitSet().set(Tequila) |
                                   IngredientBitSet().set(LimeJuice) |
                                   IngredientBitSet().set(TripleSec);

const IngredientBitSet COSMO =
    IngredientBitSet().set(Vodka) | IngredientBitSet().set(CranJuice) |
    IngredientBitSet().set(LimeJuice) | IngredientBitSet().set(TripleSec);

const IngredientBitSet MOJITO =
    IngredientBitSet().set(Rum) | IngredientBitSet().set(LimeJuice) |
    IngredientBitSet().set(Soda) | IngredientBitSet().set(Water) |
    IngredientBitSet().set(MintLeaf) | IngredientBitSet().set(SimpleSyrup);

const IngredientBitSet OLD_FASH = IngredientBitSet().set(Whiskey) |
                                  IngredientBitSet().set(Bitters) |
                                  IngredientBitSet().set(SimpleSyrup);

const IngredientBitSet DAIQUIRI = IngredientBitSet().set(Rum) |
                                  IngredientBitSet().set(LimeJuice) |
                                  IngredientBitSet().set(SimpleSyrup);

const IngredientBitSet PINA_COLADA = IngredientBitSet().set(Rum) |
                                     IngredientBitSet().set(PinaJuice) |
                                     IngredientBitSet().set(CoconutCream);

const IngredientBitSet G_AND_T = IngredientBitSet().set(Gin) |
                                 IngredientBitSet().set(Tonic) |
                                 IngredientBitSet().set(Lime);

const IngredientBitSet WHISKEY_SOUR = IngredientBitSet().set(Whiskey) |
                                      IngredientBitSet().set(LemonJuice) |
                                      IngredientBitSet().set(SimpleSyrup);

const IngredientBitSet VODKA_TONIC = IngredientBitSet().set(Vodka) |
                                     IngredientBitSet().set(Tonic) |
                                     IngredientBitSet().set(Lime);

}  // namespace recipe
