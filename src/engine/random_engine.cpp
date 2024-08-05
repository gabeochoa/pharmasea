
#include "random_engine.h"

#include "log.h"

RandomEngine RandomEngine::instance;
bool RandomEngine::created = false;

void RandomEngine::create() {
    if (created) {
        log_error(
            "Trying to create random engine twice, just reseed if you need");
        return;
    }
    new (&instance) RandomEngine();
    created = true;
}

RandomEngine& RandomEngine::get() {
    if (!created) {
        log_warn("Creating random engine manually...");
        create();
        set_seed(instance.seed);
    }
    return instance;
}

void RandomEngine::set_seed(const std::string& new_seed) {
    if (!created) {
        log_warn("Creating random engine manually...");
        create();
    }
    instance._set_seed(new_seed);
}

void RandomEngine::_set_seed(const std::string& new_seed) {
    seed = new_seed;
    hashed_seed = hashString(seed);
    log_info("Initializing random engine with seed: {}({})", new_seed,
             hashed_seed);
    // reset the local engine
    rng_engine = pcg32(hashed_seed);
    int_dist = std::uniform_int_distribution<int>(1, 1000);
    float_dist = std::uniform_real_distribution<float>(0, 1);
}

bool RandomEngine::get_bool() { return get_int(0, 1) == 1; }
int RandomEngine::get_sign() { return get_bool() ? -1 : 1; }
std::string RandomEngine::get_string(int length) {
    std::string out(length, '\0');
    for (int i = 0; i < length; i++) {
        out[i] = (char) (get_int('a', 'z'));
    }
    return out;
}

int RandomEngine::get_int(int a, int b) {
    return a + (int_dist(rng_engine) % (b - a + 1));
}

float RandomEngine::get_float(float a, float b) {
    return (a + (b - a)) * float_dist(rng_engine);
}

vec2 RandomEngine::get_vec(float mn, float mx) {
    float a = get_float(mn, mx);
    float b = get_float(mn, mx);
    return vec2{a, b};
}

vec2 RandomEngine::get_vec(float mn_a, float mx_a, float mn_b, float mx_b) {
    float a = get_float(mn_a, mx_a);
    float b = get_float(mn_b, mx_b);
    return vec2{a, b};
}
