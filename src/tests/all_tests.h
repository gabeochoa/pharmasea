
#pragma once

/*


   Your test file should match test_<module being tested>.h
   and the function that has all the tests should be the same as well

   Please dont print during tests as these run on startup
*/

#include "component.h"
#include "lerp_test.h"
#include "test_ui_widget.h"

namespace tests {

void run_all() {
    test_ui_widget();
    all_tests();

    lerp_test();
}

}  // namespace tests
