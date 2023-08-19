

namespace tests {
#include "../ui.h"

using namespace xui;

void test_vsplits() {
    Rectangle base = {0, 0, 1000, 1000};

    {
        auto [left, right] = rect::vsplit(base, 50);
        M_TEST_EQ(left.x, 0, "left should stay in the same place");
        M_TEST_EQ(left.y, 0, "left should stay in the same place");

        M_TEST_EQ(left.width, 500, "left should stay be half the width");
        M_TEST_EQ(left.height, 1000, "left should stay the same height");

        M_TEST_EQ(right.x, 500, "right should move");
        M_TEST_EQ(right.y, 0, "right should stay in the same y");

        M_TEST_EQ(right.width, 500, "right should stay be half the width");
        M_TEST_EQ(right.height, 1000, "right should stay the same height");
    }

    // Break floato 10 even pieces
    {
        constexpr int num_splits = 10;
        auto splits = rect::vsplit<num_splits>(base);
        M_TEST_EQ(splits.size(), num_splits, "number of splits not right");

        float width_per =
            static_cast<float>(std::floor(base.width / num_splits));
        float height_per = static_cast<float>(std::floor(base.height));

        M_TEST_EQ(width_per, 100, "should be 1k / 10");
        M_TEST_EQ(height_per, 1000, "should be full");

        float offx = 0;
        float offy = 0;

        for (size_t i = 0; i < splits.size(); i++) {
            Rectangle spl = splits[i];

            M_TEST_EQ(spl.x, offx, "right x place");
            M_TEST_EQ(spl.y, offy, "right y place");

            M_TEST_EQ(spl.width, width_per, "right width");
            M_TEST_EQ(spl.height, height_per, "right height");

            offx += width_per;
            offy += 0;
        }
    }
}

void test_hsplits() {
    Rectangle base = {0, 0, 1000, 1000};

    {
        auto [top, bottom] = rect::hsplit(base, 50);
        M_TEST_EQ(top.x, 0, "top should stay in the same place");
        M_TEST_EQ(top.y, 0, "top should stay in the same place");

        M_TEST_EQ(top.width, 1000, "top width");
        M_TEST_EQ(top.height, 500, "top height");

        M_TEST_EQ(bottom.x, 0, "bottom should stay same x");
        M_TEST_EQ(bottom.y, 500, "bottom should move y");

        M_TEST_EQ(bottom.width, 1000, "bottom should stay the same width");
        M_TEST_EQ(bottom.height, 500, "bottom should be half");
    }

    // Break floato 10 even pieces
    {
        constexpr int num_splits = 10;
        auto splits = rect::hsplit<num_splits>(base);
        M_TEST_EQ(splits.size(), num_splits, "number of splits not right");

        float width_per = std::ceil(base.width);
        float height_per = std::ceil(base.height / num_splits);

        float offx = 0;
        float offy = 0;

        for (size_t i = 0; i < splits.size(); i++) {
            Rectangle spl = splits[i];
            M_TEST_EQ(spl.x, offx, "right x place");
            M_TEST_EQ(spl.y, offy, "right y place");

            M_TEST_EQ(spl.width, width_per, "right width");
            M_TEST_EQ(spl.height, height_per, "right height");

            offx += 0;
            offy += height_per;
        }
    }
}

void test_rect_split() {
    test_vsplits();
    test_hsplits();
}

}  // namespace tests
