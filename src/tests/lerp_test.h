

#pragma once

#include "../vec_util.h"

namespace tests {

void test_half() {
    vec2 a{0, 0};
    vec2 b{1, 1};
    vec2 c = vec::lerp(a, b, 0.5f);

    M_TEST_EQ(c.x, 0.5, "should be half");
    M_TEST_EQ(c.y, 0.5, "should be half");
}

void test_tenth() {
    vec2 a{0, 0};
    vec2 b{10, 10};
    vec2 c = vec::lerp(a, b, 0.1f);

    M_TEST_EQ(c.x, 1, "should be tenth");
    M_TEST_EQ(c.y, 1, "should be tenth");
}

void lerp_test() {
    test_half();
    test_tenth();
}

}  // namespace tests
