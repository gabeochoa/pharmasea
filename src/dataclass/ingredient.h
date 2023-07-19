
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
    // Lime,      // TODO we dont currently have a lime model, so everythign is
    // LimeJuice, // lemon

    // Juice,
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
                                   // TODO this is actually supposdd to be lime
                                   // juice IngredientBitSet().set(LimeJuice) |
                                   IngredientBitSet().set(LemonJuice) |
                                   IngredientBitSet().set(TripleSec);

const IngredientBitSet COSMO =
    IngredientBitSet().set(Vodka) | IngredientBitSet().set(CranJuice) |
    // TODO this is actually supposdd to be lime juice
    // IngredientBitSet().set(LimeJuice) |
    IngredientBitSet().set(LemonJuice) | IngredientBitSet().set(TripleSec);

const IngredientBitSet MOJITO =
    IngredientBitSet().set(Rum) |
    // TODO this is actually supposdd to be lime juice
    // IngredientBitSet().set(LimeJuice) |
    IngredientBitSet().set(LemonJuice) | IngredientBitSet().set(Soda) |
    // TODO :SODAWAND: i really want you to have to do it twice but for now w'll
    // just ignore
    //
    // IngredientBitSet().set(Water) |
    IngredientBitSet().set(MintLeaf) | IngredientBitSet().set(SimpleSyrup);

const IngredientBitSet OLD_FASH = IngredientBitSet().set(Whiskey) |
                                  IngredientBitSet().set(Bitters) |
                                  IngredientBitSet().set(SimpleSyrup);

const IngredientBitSet DAIQUIRI = IngredientBitSet().set(Rum) |
                                  // TODO this is actually supposdd to be lime
                                  // juice
                                  //
                                  // IngredientBitSet().set(LimeJuice) |
                                  IngredientBitSet().set(LemonJuice) |
                                  IngredientBitSet().set(SimpleSyrup);

const IngredientBitSet PINA_COLADA = IngredientBitSet().set(Rum) |
                                     IngredientBitSet().set(PinaJuice) |
                                     IngredientBitSet().set(CoconutCream);

const IngredientBitSet G_AND_T = IngredientBitSet().set(Gin) |
                                 // TODO See :SODAWAND: for more info
                                 // IngredientBitSet().set(Tonic) |
                                 IngredientBitSet().set(Soda) |
                                 // TODO lime
                                 // IngredientBitSet().set(Lime);
                                 IngredientBitSet().set(Lemon);

const IngredientBitSet WHISKEY_SOUR = IngredientBitSet().set(Whiskey) |
                                      IngredientBitSet().set(LemonJuice) |
                                      IngredientBitSet().set(SimpleSyrup);

const IngredientBitSet VODKA_TONIC = IngredientBitSet().set(Vodka) |
                                     // TODO See :SODAWAND: for more info
                                     // IngredientBitSet().set(Tonic) |
                                     IngredientBitSet().set(Soda) |
                                     // TODO lime
                                     // IngredientBitSet().set(Lime);
                                     IngredientBitSet().set(Lemon);

}  // namespace recipe
