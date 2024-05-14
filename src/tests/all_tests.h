
#pragma once

/*


   Your test file should match test_<module being tested>.h
   and the function that has all the tests should be the same as well

   Please dont print during tests as these run on startup
*/

#include "component.h"
#include "lerp_test.h"
#include "rect_split_tests.h"
#include "size_ents.h"
#include "test_pathing.h"
#include "test_ui_widget.h"

namespace tests {

inline void run_all() {
    // log nothing during the test
    auto old_level = LOG_LEVEL;
    LOG_LEVEL = 6;

    test_ui_widget();
    all_tests();

    lerp_test();
    test_all_pathing();

    size_test();
    test_rect_split();

    // back to default , preload will set it as well
    LOG_LEVEL = old_level;
}

}  // namespace tests
