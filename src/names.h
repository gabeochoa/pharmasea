#pragma once
// a list of names to pull from name generator

#include "external_include.h"
#include "random.h"

const std::array<const char*, 19> first_names = {
    "Buck", "Bark",  "Cleef", "Tuck", "Jack", "Sal",  "Clove",
    "Beef", "Bart",  "Ted",   "Ned",  "Kyle", "Myle", "Mark",
    "Jerk", "Bjork", "Steve", "Stan", "Bran"};

const std::array<const char*, 8> last_names = {"Buckle", "Chuckle", "Stedman",
                                               "Slick",  "Fischer", "Flipper",
                                               "Amore",  "Fishman"};

static std::string get_random_name() {
    int first = randIn(0, first_names.size() - 1);
    int last = randIn(0, last_names.size() - 1);
    return fmt::format("{} {}", first_names[first], last_names[last]);
}

// TODO full names (special customers/doctors?)
//  Dr. Sturgeon
//  Herman Merman
//  Doctopus
//  Gordon Freewilly
