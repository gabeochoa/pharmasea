#pragma once
// a list of names to pull from name generator

#include <string>

#include "../engine/random_engine.h"
#include "../external_include.h"

constexpr std::array<const char*, 39> alcohol_names = {
    "Martini", "Ale",       "Chardonnay", "Brandy",   "Tequila",  "Lager",
    "Rosé",    "Mojito",    "Whiskey",    "Gin",      "Mimosa",   "Rum",
    "Bourbon", "Shiraz",    "Margarita",  "Merlot",   "Cosmo",    "Aperol",
    "Shandy",  "Bellini",   "Cider",      "Julep",    "Cabernet", "Sambuca",
    "Rye",     "Pinot",     "Fizz",       "Daiquiri", "Pina",     "Sangria",
    "Scotch",  "Zinfandel", "Prosecco",   "Cognac",   "Kahlua",   "Vermouth",
    "Fernet",  "Sake",      "Jager",
};

constexpr std::array<const char*, 42> alcy_firsts = {
    "Martina", "Al",       "Chardonnay", "Brandy", "Rose",    "Jim",
    "Ron",     "Ben",      "Shira",      "Marge",  "Cosmo",   "Lagertha",
    "April",   "Randy",    "Belinda",    "Julie",  "Renee",   "Sam",
    "Ryan",    "Fez",      "Zachary",    "Tina",   "Scott",   "Sandy",
    "Jager",   "Rosemary", "Tito",       "Jim",    "Sheila",  "Vito",
    "Daniel",  "Morgan",   "Johnnie",    "Walker", "Jose",    "Stella",
    "Samuel",  "Adam",     "Mike",       "Miller", "Jameson", "Hendrick",

};

constexpr std::array<const char*, 19> first_names = {
    "Buck", "Bark",  "Cleef", "Tuck", "Jack", "Sal",  "Clove",
    "Beef", "Bart",  "Ted",   "Ned",  "Kyle", "Myle", "Mark",
    "Jerk", "Bjork", "Steve", "Stan", "Bran"};

constexpr std::array<const char*, 8> last_names = {
    "Buckle",  "Chuckle", "Stedman", "Slick",
    "Fischer", "Flipper", "Amore",   "Fishman"};

static std::string get_random_name() {
    int first = RandomEngine::get().get_index(alcy_firsts);
    int last = RandomEngine::get().get_index(alcohol_names);
    return fmt::format("{} {}", alcy_firsts[first], alcohol_names[last]);
}

static const char* rot13(const char* input) {
    size_t length = strlen(input);
    char* result = new char[length + 1];

    for (size_t i = 0; i < length; ++i) {
        char c = input[i];

        if ('a' <= c && c <= 'z') {
            // convert to uppercase
            result[i] = ('A' + (c - 'a' + 13) % 26);
        } else if ('A' <= c && c <= 'Z') {
            result[i] = 'A' + (c - 'A' + 13) % 26;
        } else {
            result[i] = c;  // Preserve non-alphabetic characters
        }
    }

    result[length] = '\0';  // Null-terminate the result string
    return result;
}

static std::string get_random_name_rot13() {
    int first = RandomEngine::get().get_index(alcy_firsts);
    int last = RandomEngine::get().get_index(alcohol_names);
    return fmt::format("{}_{}", rot13(alcy_firsts[first]),
                       rot13(alcohol_names[last]));
}
