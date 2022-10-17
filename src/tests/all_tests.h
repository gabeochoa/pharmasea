
#pragma once

/*


   Your test file should match test_<module being tested>.h
   and the function that has all the tests should be the same as well

   Please dont print during tests as these run on startup
*/

#include "test_entity_fetching.h"
#include "test_ui_widget.h"

namespace tests {

void run_all() {
    test_ui_widget();
    test_entity_fetching();
}

}  // namespace tests
