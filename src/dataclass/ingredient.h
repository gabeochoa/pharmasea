
#pragma once

#include <bitset>
#include <magic_enum/magic_enum.hpp>

#include "../engine/constexpr_containers.h"
#include "../engine/random.h"

enum Ingredient {
    Invalid,

    IceCubes,
    IceCrushed,

    // Non Alcoholic
    Soda,
    // Water, //< Soda
    // Tonic, //< Soda

    // Garnishes
    Salt,
    MintLeaf,

    // Liquor
    Rum,
    Tequila,  // Silver or gold?
    Vodka,
    Whiskey,  // Rye or Bourbon?
    Gin,
    TripleSec,
    // Cointreau, < TripleSec
    Bitters,

    // Lemon
    Lemon,
    LemonJuice,
    Lime,
    LimeJuice,
    Orange,
    OrangeJuice,
    Cranberries,
    CranberryJuice,
    Coconut,
    CoconutCream,
    // PinaJuice, //< lemonjuice

    SimpleSyrup,
};

namespace ingredient {
constexpr std::array<Ingredient, 7> Alcohols = {{
    Rum,
    Tequila,
    Vodka,
    Whiskey,
    Gin,
    TripleSec,
    Bitters,
}};

constexpr std::array<Ingredient, 5> Fruits = {
    {Lemon, Lime, Cranberries, Coconut, Orange}};

constexpr CEMap<Ingredient, Ingredient, 5> BlendConvert = {{{
    {Lemon, LemonJuice},
    {Lime, LimeJuice},
    {Cranberries, CranberryJuice},
    {Coconut, CoconutCream},
    {Orange, OrangeJuice},
}}};

}  // namespace ingredient

static Ingredient get_ingredient_from_index(int index) {
    return magic_enum::enum_cast<Ingredient>(index).value();
}

const int MAX_INGREDIENT_TYPES = 32;
using IngredientBitSet = std::bitset<MAX_INGREDIENT_TYPES>;

enum Drink {
    coke,
    rum_and_coke,
    margarita,
    daiquiri,
    gin_and_tonic,
    whiskey_sour,
    vodka_tonic,
    old_fashioned,
    cosmo,
    mojito,
    pina_colada,
    screwdriver,
    moscow_mule,
    long_island,
    mai_tai,
};

using DrinkSet = std::bitset<magic_enum::enum_count<Drink>()>;
