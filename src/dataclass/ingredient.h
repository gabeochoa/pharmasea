
#pragma once

#include <bitset>
#include <magic_enum/magic_enum.hpp>

#include "../engine/random.h"

enum Ingredient {
    Invalid = -1,

    IceCubes,
    IceCrushed,

    // Non Alcoholic
    Soda,
    Water,
    Tonic = Soda,

    // Garnishes
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
    LAST_ALC,

    // Lemon
    Lemon,
    LemonJuice,
    Lime = Lemon,
    LimeJuice = LemonJuice,

    // Juice,
    CranJuice = Soda,
    PinaJuice = Soda,
    CoconutCream = Soda,
    SimpleSyrup,

};

namespace ingredient {
const Ingredient ALC_START = Ingredient::Rum;
const Ingredient ALC_END = LAST_ALC;
const int NUM_ALC = (ALC_END - ALC_START);

const Ingredient LEMON_START = Ingredient::Lemon;
const Ingredient LEMON_END = Ingredient::LemonJuice;
const int NUM_LEMON = (LEMON_END - LEMON_START) + 1;

}  // namespace ingredient

static Ingredient get_ingredient_from_index(int index) {
    return magic_enum::enum_cast<Ingredient>(index).value();
}

const int MAX_INGREDIENT_TYPES = 32;
using IngredientBitSet = std::bitset<MAX_INGREDIENT_TYPES>;

namespace recipe {

const IngredientBitSet MARGARITA = IngredientBitSet().set(Tequila) |
                                   IngredientBitSet().set(LimeJuice) |
                                   IngredientBitSet().set(TripleSec);

const IngredientBitSet COSMO =
    IngredientBitSet().set(Vodka) | IngredientBitSet().set(CranJuice) |
    IngredientBitSet().set(LimeJuice) | IngredientBitSet().set(TripleSec);

const IngredientBitSet MOJITO =
    IngredientBitSet().set(Rum) | IngredientBitSet().set(LimeJuice) |
    IngredientBitSet().set(Soda) |
    // TODO :SODAWAND: i really want you to have to do it twice but for now w'll
    // just ignore
    //
    // IngredientBitSet().set(Water) |
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
   //

enum Drink {
    coke,
    rum_and_coke,
    Margarita,
    Daiquiri,
    GAndT,
    WhiskeySour,
    VodkaTonic,
    //
    LAST_DRINK,
    //
    OldFash,
    Cosmo,
    Mojito,
    PinaColada,
};

static Drink get_random_drink() {
    int index = randIn(0, Drink::LAST_DRINK);
    return magic_enum::enum_cast<Drink>(index).value();
}
