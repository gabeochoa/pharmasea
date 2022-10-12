
#include "../globals.h"
#include "../ui_widget.h"

namespace tests {

void test_empty() {
    using namespace ui;
    Widget root;
    process_widget(&root);
}

void test_just_root() {
    using namespace ui;
    Widget root;
    root.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = WIN_W, .strictness = 1.f};
    root.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Pixels, .value = WIN_H, .strictness = 1.f};
    process_widget(&root);

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

    root.first = &child1;
    root.last = &child1;

    root.next = &child1;

    child1.parent = &root;

    process_widget(&root);

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

    root.first = &child1;
    root.last = &child2;
    root.next = &child1;

    child1.parent = &root;
    child2.parent = &root;

    child1.next = &child2;
    child2.prev = &child1;

    process_widget(&root);

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

    root.first = &child1;
    root.last = &child2;
    root.next = &child1;

    child1.parent = &root;
    child2.parent = &root;

    child1.next = &child2;
    child2.prev = &child1;

    process_widget(&root);

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

    root.first = &child1;
    root.last = &child2;
    root.next = &child1;

    child1.parent = &root;
    child2.parent = &root;

    child1.next = &child2;
    child2.prev = &child1;

    process_widget(&root);

    // std::cout << child1 << std::endl;
    // std::cout << child2 << std::endl;

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
    M_TEST_EQ(root.first, &child, "Parent of child should be root");
    M_TEST_EQ(root.last, &child,
              "Last child of root should be child since there is only one");
    M_TEST_EQ(root.first->next, nullptr, "root should only have one child");
    M_TEST_EQ(child.next, nullptr, "child should have no siblings");
    M_TEST_EQ(child.next, root.first->next,
              "root's first child's next should be same as child's next");
}

void test_ui_widget() {
    test_empty();
    test_just_root();
    test_single_child();
    test_two_children();
    test_two_children_parent_size();
    test_two_children_parent_size_too_small();
    test_add_child();
}
}  // namespace tests
