
#pragma once

#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#pragma clang diagnostic ignored "-Wdeprecated-volatile"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#ifdef WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

//
#include <pcg_random/pcg_extras.hpp>
#include <pcg_random/pcg_random.hpp>
//
#ifdef __APPLE__
#pragma clang diagnostic pop
#else
#pragma enable_warn
#endif

#ifdef WIN32
#pragma GCC diagnostic pop
#endif

#ifdef _MSC_VER
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#include <random>
#include <string>

#include "../vendor_include.h"

struct RandomEngine {
    static void create();
    [[nodiscard]] static RandomEngine& get();
    static void set_seed(const std::string& new_seed);

    [[nodiscard]] bool get_bool();
    [[nodiscard]] int get_sign();
    [[nodiscard]] std::string get_string(int length);
    [[nodiscard]] int get_int(int a, int b);
    [[nodiscard]] float get_float(float a, float b);
    [[nodiscard]] vec2 get_vec(float mn, float mx);

    template<typename T>
    [[nodiscard]] int get_index(const T& list) {
        return get_int(0, static_cast<int>(list.size()) - 1);
    }

    [[nodiscard]] static pcg32 generator() { return instance.rng_engine; }

   private:
    void _set_seed(const std::string& new_seed);
    std::string seed = "default_seed";
    size_t hashed_seed;
    pcg32 rng_engine;
    std::uniform_int_distribution<int> int_dist;
    std::uniform_real_distribution<float> float_dist;

    // singleton stuff
    static RandomEngine instance;
    static bool created;
};
