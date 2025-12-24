
#pragma once

/*


   Your test file should match test_<module being tested>.h
   and the function that has all the tests should be the same as well

   Please dont print during tests as these run on startup
*/

#include "../engine/assert.h"
#include "component.h"
#include "lerp_test.h"
#include "rect_split_tests.h"
#include "size_ents.h"
#include "test_entity_serialization.h"
#include "test_map_playability.h"
#include "test_pathing.h"
#include "test_ui_widget.h"

namespace tests {

inline void random_tests() {
    RandomEngine::set_seed("test");

    const auto get_and_validate = []<typename T>(const T& a, const T& b) {
        VALIDATE(a == b,
                 fmt::format("Expected value {} to be {} but was not", a, b));
    };

    get_and_validate(RandomEngine::get().get_int(0, 10), 10);
    get_and_validate(RandomEngine::get().get_int(0, 10), 8);
    get_and_validate(RandomEngine::get().get_bool(), true);
    get_and_validate(
        util::round_nearest(RandomEngine::get().get_float(0.f, 1.f), 1), 0.5f);
    get_and_validate(
        util::round_nearest(RandomEngine::get().get_float(0.f, 10.f), 1), 2.8f);
    get_and_validate(
        util::round_nearest(RandomEngine::get().get_float(0.f, 100.f), 1),
        29.9f);
    get_and_validate(
        util::round_nearest(RandomEngine::get().get_vec(0.f, 1.f).x, 2), 0.26f);
    //

    get_and_validate(RandomEngine::get().get_int(0, 10), 7);
    get_and_validate(RandomEngine::get().get_int(0, 10), 0);
    get_and_validate(RandomEngine::get().get_bool(), true);
    get_and_validate(
        util::round_nearest(RandomEngine::get().get_float(0.f, 1.f), 1), 0.6f);
    get_and_validate(
        util::round_nearest(RandomEngine::get().get_float(0.f, 10.f), 1), 6.9f);
    get_and_validate(
        util::round_nearest(RandomEngine::get().get_float(0.f, 100.f), 1),
        16.2f);
    get_and_validate(
        util::round_nearest(RandomEngine::get().get_vec(0.f, 1.f).x, 2), 0.82f);
}

inline void run_all() {
    // log nothing during the test
    auto old_level = LOG_LEVEL;
    LOG_LEVEL = 6;

    random_tests();

    test_ui_widget();
    all_tests();

    lerp_test();
    test_map_playability();
    test_all_pathing();

    size_test();
    test_rect_split();
    test_entity_serialization();

    // back to default , preload will set it as well
    LOG_LEVEL = old_level;
}

}  // namespace tests
