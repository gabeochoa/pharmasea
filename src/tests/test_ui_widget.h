
#include "../assert.h"
#include "../globals.h"
#include "../ui_autolayout.h"
#include "../ui_widget.h"

namespace tests {

void test_empty() {
    using namespace ui;
    Widget root;
    autolayout::process_widget(&root);
}

void test_just_root() {
    using namespace ui;
    Widget root;
    root.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = WIN_W, .strictness = 1.f};
    root.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = WIN_H, .strictness = 1.f};
    autolayout::process_widget(&root);

    M_TEST_EQ(root.computed_size[0], WIN_W, "value should match exactly");
    M_TEST_EQ(root.computed_size[1], WIN_H, "value should match exactly");
}

void test_single_child() {
    using namespace ui;
    Widget root;
    root.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = WIN_W, .strictness = 1.f};
    root.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = WIN_H, .strictness = 1.f};

    Widget child1;
    child1.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Percent, .value = 0.5f, .strictness = 1.f};
    child1.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Percent, .value = 0.5f, .strictness = 1.f};

    root.add_child(&child1);

    autolayout::process_widget(&root);

    M_TEST_EQ(root.computed_size[0], WIN_W, "value should match exactly");
    M_TEST_EQ(root.computed_size[1], WIN_H, "value should match exactly");

    M_TEST_EQ(child1.computed_size[0], WIN_W * 0.5f,
              "value should match exactly");
    M_TEST_EQ(child1.computed_size[1], WIN_H * 0.5f,
              "value should match exactly");
}

void test_two_children() {
    using namespace ui;
    Widget root;

    root.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = WIN_W, .strictness = 1.f};
    root.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = WIN_H, .strictness = 1.f};

    Widget child1;
    child1.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Percent, .value = 0.5f, .strictness = 1.f};
    child1.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Percent, .value = 0.5f, .strictness = 1.f};

    Widget child2;
    child2.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Percent, .value = 0.5f, .strictness = 1.f};
    child2.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Percent, .value = 0.25f, .strictness = 1.f};

    root.add_child(&child1);
    root.add_child(&child2);

    autolayout::process_widget(&root);

    M_TEST_EQ(root.computed_size[0], WIN_W, "value should match exactly");
    M_TEST_EQ(root.computed_size[1], WIN_H, "value should match exactly");

    M_TEST_EQ(child1.computed_size[0], WIN_W * 0.5f,
              "value should match exactly");
    M_TEST_EQ(child1.computed_size[1], WIN_H * 0.5f,
              "value should match exactly");

    M_TEST_EQ(child2.computed_size[0], WIN_W * 0.5f,
              "value should match exactly");
    M_TEST_EQ(child2.computed_size[1], WIN_H * 0.25f,
              "value should match exactly");
}

void test_two_children_parent_size() {
    using namespace ui;
    Widget root;

    root.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Children, .value = WIN_W, .strictness = 1.f};
    root.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = WIN_H, .strictness = 1.f};

    Widget child1;
    child1.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 100.f, .strictness = 1.f};
    child1.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 100.f, .strictness = 1.f};

    Widget child2;
    child2.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 50.f, .strictness = 1.f};
    child2.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 25.f, .strictness = 1.f};

    root.add_child(&child1);
    root.add_child(&child2);

    autolayout::process_widget(&root);

    M_TEST_EQ(root.computed_size[0],
              child1.computed_size[0] + child2.computed_size[0],
              "value should match size of both children together");
    M_TEST_EQ(root.computed_size[1], WIN_H,
              "value should match size in pixels");

    M_TEST_EQ(child1.computed_size[0], 100.f, "value should match exactly");
    M_TEST_EQ(child1.computed_size[1], 100.f, "value should match exactly");

    M_TEST_EQ(child2.computed_size[0], 50.f, "value should match exactly");
    M_TEST_EQ(child2.computed_size[1], 25.f, "value should match exactly");
}

void test_two_children_parent_size_too_small() {
    using namespace ui;
    Widget root;

    root.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 100.f, .strictness = 1.f};
    root.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 100.f, .strictness = 1.f};

    Widget child1;
    child1.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 200.f, .strictness = 0.1f};
    child1.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 200.f, .strictness = 0.1f};

    Widget child2;
    child2.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 99.f, .strictness = 1.f};
    child2.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 99.f, .strictness = 1.f};

    root.add_child(&child1);
    root.add_child(&child2);

    autolayout::process_widget(&root);

    M_TEST_EQ(root.computed_size[0], 100.f, "value should match size exactly");
    M_TEST_EQ(root.computed_size[1], 100.f, "value should match size exactly");

    M_TEST_LEQ(child1.computed_size[0], 100.f,
               "value should be smaller than max size");
    M_TEST_LEQ(child1.computed_size[1], 100.f,
               "value should be smaller than max size");
    M_TEST_LEQ(child2.computed_size[0], 100.f,
               "value should be smaller than max size");
    M_TEST_LEQ(child2.computed_size[1], 100.f,
               "value should be smaller than max size");

    M_TEST_LEQ(child1.computed_size[0] + child2.computed_size[0], 100.f,
               "value of all children should be , max size");
    M_TEST_LEQ(child1.computed_size[1] + child2.computed_size[1], 100.f,
               "value of all children should be , max size");

    M_TEST_LEQ(child1.computed_size[1], 200.f,
               "<1.f strictness could have been compromised on");
    M_TEST_EQ(child2.computed_size[1], 99.f,
              "1.f strictness should be enforced");
}

void test_add_child() {
    using namespace ui;
    Widget root;

    root.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 100.f, .strictness = 1.f};
    root.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 100.f, .strictness = 1.f};

    Widget child;
    child.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 200.f, .strictness = 0.1f};
    child.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 200.f, .strictness = 0.1f};

    root.add_child(&child);

    M_TEST_EQ(child.parent, &root, "Parent of child should be root");
    M_TEST_EQ(*root.children.begin(), &child, "Parent of child should be root");
    M_TEST_EQ(*root.children.rbegin(), &child,
              "Last child of root should be child since there is only one");
    M_TEST_EQ(root.children.size(), 1, "root should only have one child");
}

void test_add_child_again() {
    using namespace ui;
    Widget root;

    Widget child;
    root.add_child(&child);

    Widget child2;
    root.add_child(&child2);

    M_TEST_EQ(child.parent, &root, "Parent of child should be root");
    M_TEST_EQ(*root.children.begin(), &child, "Parent of child should be root");

    M_TEST_NEQ(*root.children.rbegin(), &child,
               "Last child of root should not be child since there is two ");
    M_TEST_EQ(*root.children.rbegin(), &child2,
              "Last child of root should be child2 since there is two ");
    M_TEST_NEQ(root.children.size(), 1, "root should have more than one child");
    M_TEST_EQ(root.children.size(), 2, "root should have two children");
}

void test_autolayout_wrap_column() {
    using namespace ui;
    Widget root;

    root.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 100.f, .strictness = 1.f};
    root.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 50.f, .strictness = 1.f};
    root.growflags = GrowFlags::Column;

    Widget column1;
    column1.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 25.f, .strictness = 1.f};
    column1.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 25.f, .strictness = 1.f};
    root.add_child(&column1);

    Widget column2a;
    column2a.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 75.f, .strictness = 1.f};
    column2a.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 45.f, .strictness = 1.f};
    root.add_child(&column2a);

    // Note that 2b does fit vertically and is less wide than 2a;
    Widget column2b;
    column2b.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 25.f, .strictness = 1.f};
    column2b.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 5.f, .strictness = 1.f};
    root.add_child(&column2b);

    Widget column3;
    column3.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 25.f, .strictness = 1.f};
    column3.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = 50.f, .strictness = 1.f};
    root.add_child(&column3);

    autolayout::process_widget(&root);

    // std::cout << root << std::endl;
    // std::cout << column1 << std::endl;
    // std::cout << column2a << std::endl;
    // std::cout << column2b << std::endl;
    // std::cout << column3 << std::endl;

    M_TEST_EQ(column1.parent, &root, "Parent of column1 should be root");
    M_TEST_EQ(*root.children.begin(), &column1,
              "Parent of column1 should be root");
    M_TEST_EQ(*root.children.rbegin(), &column3,
              "Last column1 of root should be column3");
    M_TEST_EQ(root.children.size(), 4, "root should only have three children");

    M_TEST_EQ(root.computed_size[0], 100, "Root should be 100 px wide");
    M_TEST_EQ(root.computed_size[1], 50, "Root should be 50 px tall");

    M_TEST_EQ(column1.computed_size[0], 25, "Root should be 25 px wide");
    M_TEST_EQ(column1.computed_size[1], 25, "Root should be 25 px tall");

    M_TEST_EQ(column2a.computed_size[0], 75, "column2a should be 75 px wide");
    M_TEST_EQ(column2a.computed_size[1], 45, "column2a should be 45 px tall");

    M_TEST_EQ(column2b.computed_size[0], 25, "column2b should be 25 px wide");
    M_TEST_EQ(column2b.computed_size[1], 5, "column2b should be 5 px tall");

    M_TEST_EQ(column3.computed_size[0], 25, "column3 should be 25 px wide");
    M_TEST_EQ(column3.computed_size[1], 50, "column3 should be 50 px tall");

    M_TEST_EQ(root.computed_relative_pos[0], 0, "Root should be at 0 x");
    M_TEST_EQ(root.computed_relative_pos[1], 0, "Root should be at 0 y");

    M_TEST_EQ(column1.computed_relative_pos[0], 0, "column1 should be at 0 x");
    M_TEST_EQ(column1.computed_relative_pos[1], 0, "column1 should be at 0 y");

    M_TEST_EQ(column2a.computed_relative_pos[0], 25,
              "column2a should be at 25 x");
    M_TEST_EQ(column2a.computed_relative_pos[1], 0,
              "column2a should be at 0 y");

    M_TEST_EQ(column2b.computed_relative_pos[0], 25,
              "column2b should be at 25 x");
    M_TEST_EQ(column2b.computed_relative_pos[1], 45,
              "column2b should be at 45 y");

    M_TEST_EQ(column3.computed_relative_pos[0], 100,
              "column3 should be at 100 x");
    M_TEST_EQ(column3.computed_relative_pos[1], 0, "column3 should be at 0 y");
}

void test_ui_widget() {
    test_empty();
    test_just_root();
    test_single_child();
    test_two_children();
    test_two_children_parent_size();
    test_two_children_parent_size_too_small();
    test_add_child();
    test_add_child_again();
    test_autolayout_wrap_column();
}
}  // namespace tests
