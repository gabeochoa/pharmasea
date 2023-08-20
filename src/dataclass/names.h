#pragma once
// a list of names to pull from name generator

#include "../engine/random.h"
#include "../external_include.h"

const std::array<const char*, 40> alcohol_names = {
    "Martini", "Ale",       "Chardonnay", "Brandy",   "Tequila",  "Lager",
    "Rosé",    "Mojito",    "Whiskey",    "Gin",      "Mimosa",   "Rum",
    "Bourbon", "Shiraz",    "Margarita",  "Merlot",   "Cosmo",    "Aperol",
    "Shandy",  "Bellini",   "Cider",      "Julep",    "Cabernet", "Sambuca",
    "Rye",     "Pinot",     "Fizz",       "Daiquiri", "Pina",     "Sangria",
    "Scotch",  "Zinfandel", "Prosecco",   "Cognac",   "Kahlua",   "Vermouth",
    "Fernet",  "Sake",      "Jager",
};

const std::array<const char*, 42> alcy_firsts = {
    "Martina", "Al",       "Chardonnay", "Brandy", "Rose",    "Jim",
    "Ron",     "Ben",      "Shira",      "Marge",  "Cosmo",   "Lagertha",
    "April",   "Randy",    "Belinda",    "Julie",  "Renee",   "Sam",
    "Ryan",    "Fez",      "Zachary",    "Tina",   "Scott",   "Sandy",
    "Jager",   "Rosemary", "Tito",       "Jim",    "Sheila",  "Vito",
    "Daniel",  "Morgan",   "Johnnie",    "Walker", "Jose",    "Stella",
    "Samuel",  "Adam",     "Mike",       "Miller", "Jameson", "Hendrick",

};

const std::array<const char*, 19> first_names = {
    "Buck", "Bark",  "Cleef", "Tuck", "Jack", "Sal",  "Clove",
    "Beef", "Bart",  "Ted",   "Ned",  "Kyle", "Myle", "Mark",
    "Jerk", "Bjork", "Steve", "Stan", "Bran"};

const std::array<const char*, 8> last_names = {"Buckle", "Chuckle", "Stedman",
                                               "Slick",  "Fischer", "Flipper",
                                               "Amore",  "Fishman"};

static std::string get_random_name() {
    int first = randIn(0, (int) alcy_firsts.size() - 1);
    int last = randIn(0, (int) alcohol_names.size() - 1);
    return fmt::format("{} {}", alcy_firsts[first], alcohol_names[last]);
}

// TODO full names (special customers/doctors?)
//  Dr. Sturgeon
//  Herman Merman
//  Doctopus
//  Gordon Freewilly
