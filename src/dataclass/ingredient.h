
#pragma once

#include <bitset>
#include <magic_enum/magic_enum.hpp>

#include "../engine/constexpr_containers.h"

enum CupType {
    Normal,
    MultiCup,
};

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

    //
    Beer,
    Champagne,

    // Liquor
    Rum,
    Tequila,  // Silver or gold?
    Vodka,
    Whiskey,  // Rye or Bourbon?
    Gin,
    TripleSec,
    // Cointreau, < TripleSec
    Bitters,

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
    Pineapple,
    PinaJuice,

    SimpleSyrup,
};

enum IngredientSoundType {
    Solid,
    Liquid,
    Ice,
    Viscous,
};

namespace ingredient {
constexpr std::array<Ingredient, 7> AlcoholsInCycle = {{
    Rum,
    Tequila,
    Vodka,
    Whiskey,
    Gin,
    TripleSec,
    Bitters,
}};

[[nodiscard]] inline bool is_alcohol(Ingredient ig) {
    switch (ig) {
        case Invalid:
        case IceCubes:
        case IceCrushed:
        case Soda:
        case Salt:
        case MintLeaf:
        case Lemon:
        case LemonJuice:
        case Lime:
        case LimeJuice:
        case Orange:
        case OrangeJuice:
        case Cranberries:
        case CranberryJuice:
        case Coconut:
        case CoconutCream:
        case Pineapple:
        case PinaJuice:
        case SimpleSyrup:
            return false;
        case Beer:
        case Rum:
        case Tequila:
        case Vodka:
        case Whiskey:
        case Gin:
        case TripleSec:
        case Bitters:
        case Champagne:
            return true;
    }
    return false;
}

constexpr std::array<Ingredient, 6> Fruits = {
    {Lemon, Lime, Cranberries, Coconut, Orange, Pineapple}};

constexpr CEMap<Ingredient, Ingredient, Fruits.size()> BlendConvert = {{{
    {Lemon, LemonJuice},
    {Lime, LimeJuice},
    {Cranberries, CranberryJuice},
    {Coconut, CoconutCream},
    {Orange, OrangeJuice},
    {Pineapple, PinaJuice},
}}};

// We could do this in a funciton switch if we add a ton of types or new
// ingredients
constexpr CEMap<Ingredient, IngredientSoundType,
                magic_enum::enum_count<Ingredient>()>
    IngredientSoundType = {{{
        //
        {Invalid, Solid},
        {Salt, Solid},
        {MintLeaf, Solid},
        {Lemon, Solid},
        {Lime, Solid},
        {Orange, Solid},
        {Cranberries, Solid},
        {Pineapple, Solid},
        {Coconut, Solid},
        //
        {Soda, Liquid},
        {Beer, Liquid},
        {Champagne, Liquid},
        {Rum, Liquid},
        {Tequila, Liquid},
        {Vodka, Liquid},
        {Whiskey, Liquid},
        {Gin, Liquid},
        {TripleSec, Liquid},
        {Bitters, Liquid},
        {LemonJuice, Liquid},
        {LimeJuice, Liquid},
        {OrangeJuice, Liquid},
        {CranberryJuice, Liquid},
        {CoconutCream, Liquid},
        {PinaJuice, Liquid},
        //
        {IceCubes, Ice},
        {IceCrushed, Ice},
        //
        {SimpleSyrup, Viscous},
    }}};

}  // namespace ingredient

static Ingredient get_ingredient_from_index(int index) {
    return magic_enum::enum_value<Ingredient>(index);
}

using IngredientBitSet = std::bitset<magic_enum::enum_count<Ingredient>()>;

enum Drink {
    coke,
    //
    beer,
    beer_pitcher,
    champagne,
    //
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

    /*

    beer (different kinds?)
    wine
    coffee
    tea

    martini
    espresso martini
    manhattan
    spritz
    white russian
    negroni
    Midori Sour
    cuba libre (rum&coke w/ lime)
    sangria (prep early in the day and makes a lot?)
    paloma (needs grapefruit)

    // popular but i never heard of

    jagerbomb (jager and red bull?)
    green tea
    kamikaze
    dirty shirley, vodka grenadine & sprite
    malibu+pina
    grayhound
    bushwacker

    Pisco Sour
    Vesper
    Caipirinha
    Hanky-Panky
    Gimlet
    Sex on the Beach
    Mimosa
    Bellini
    Sidecar
    Irish Coffee


    // sounds good
    bay breeze


    spicy drinks - add jalapeno

    // Themed ones

    Winter Holiday
    Eggnog?
    Butter Beer


     */
};

using DrinkSet = std::bitset<magic_enum::enum_count<Drink>()>;
