#pragma once
// a list of names to pull from name generator

#include "external_include.h"
#include "random.h"

const char* first_names[] = {"Buck",  "Bark",  "Cleef", "Tuck", "Jack",
                             "Sal",   "Clove", "Beef",  "Bart", "Ted",
                             "Ned",   "Kyle",  "Myle",  "Mark", "Jerk",
                             "Bjork", "Steve", "Stan",  "Bran"};

const int num_first_names = 19;

const char* last_names[] = {"Buckle",  "Chuckle", "Stedman", "Slick",
                            "Fischer", "Flipper", "Amore",   "Fishman"};
const int num_last_names = 8;

static std::string get_random_name() {
    int first = randIn(0, num_first_names);
    int last = randIn(0, num_last_names);

    return fmt::format("{} {}", first_names[first], last_names[last]);
}

// full names (special customers/doctors?)
//  Dr. Sturgeon
//  Herman Merman
//  Doctopus
//  Gordon Freewilly
