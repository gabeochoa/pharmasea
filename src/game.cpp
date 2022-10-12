
#include "external_include.h"

// Global Defines

// uncomment to enable writing / reading files :
// settings file
// save file
// #define WRITE_FILES

///
#include "globals.h"
///

#include "app.h"
#include "menu.h"
#include "settings.h"
//
#include "aboutlayer.h"
#include "fpslayer.h"
#include "gamelayer.h"
#include "menulayer.h"
#include "menustatelayer.h"
#include "settingslayer.h"
#include "versionlayer.h"

void startup() {
    // Disable all that startup logging
    SetTraceLogLevel(LOG_WARNING);
    // Force the app to be created.
    // This unlocks GPU access so we can load textures
    App::get();

    // -------- Its unlikely anything should go above this line ----- //

    Menu::get().state = Menu::State::Game;
    // Menu::get().state = Menu::State::Root;
    // Menu::get().state = Menu::State::Settings;

    GameLayer* gamelayer = new GameLayer();
    App::get().pushLayer(gamelayer);

    AboutLayer* aboutlayer = new AboutLayer();
    App::get().pushLayer(aboutlayer);

    SettingsLayer* settingslayer = new SettingsLayer();
    App::get().pushLayer(settingslayer);

    MenuLayer* menulayer = new MenuLayer();
    App::get().pushLayer(menulayer);

    MenuStateLayer* menustatelayer = new MenuStateLayer();
    App::get().pushLayer(menustatelayer);

    FPSLayer* fpslayer = new FPSLayer();
    App::get().pushLayer(fpslayer);

    VersionLayer* versionlayer = new VersionLayer();
    App::get().pushLayer(versionlayer);

    Settings::get().load_save_file();
}

void teardown() {
#ifdef WRITE_FILES
    Settings::get().write_save_file();
#endif
}

#include "ui_widget.h"

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

    M_ASSERT(root.computed_size[0] == WIN_W, "value should match exactly");
    M_ASSERT(root.computed_size[1] == WIN_H, "value should match exactly");
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

    M_ASSERT(root.computed_size[0] == WIN_W, "value should match exactly");
    M_ASSERT(root.computed_size[1] == WIN_H, "value should match exactly");

    M_ASSERT(child1.computed_size[0] == WIN_W * 0.5f,
             "value should match exactly");
    M_ASSERT(child1.computed_size[1] == WIN_H * 0.5f,
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

    M_ASSERT(root.computed_size[0] == WIN_W, "value should match exactly");
    M_ASSERT(root.computed_size[1] == WIN_H, "value should match exactly");

    M_ASSERT(child1.computed_size[0] == WIN_W * 0.5f,
             "value should match exactly");
    M_ASSERT(child1.computed_size[1] == WIN_H * 0.5f,
             "value should match exactly");

    M_ASSERT(child2.computed_size[0] == WIN_W * 0.5f,
             "value should match exactly");
    M_ASSERT(child2.computed_size[1] == WIN_H * 0.25f,
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

    M_ASSERT(root.computed_size[0] ==
                 child1.computed_size[0] + child2.computed_size[0],
             "value should match size of both children together");
    M_ASSERT(root.computed_size[1] == WIN_H,
             "value should match size in pixels");

    M_ASSERT(child1.computed_size[0] == 100.f, "value should match exactly");
    M_ASSERT(child1.computed_size[1] == 100.f, "value should match exactly");

    M_ASSERT(child2.computed_size[0] == 50.f, "value should match exactly");
    M_ASSERT(child2.computed_size[1] == 25.f, "value should match exactly");
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

    M_ASSERT(root.computed_size[0] == 100.f, "value should match size exactly");
    M_ASSERT(root.computed_size[1] == 100.f, "value should match size exactly");

    M_ASSERT(child1.computed_size[0] <= 100.f,
             "value should be smaller than max size");
    M_ASSERT(child1.computed_size[1] <= 100.f,
             "value should be smaller than max size");
    M_ASSERT(child2.computed_size[0] <= 100.f,
             "value should be smaller than max size");
    M_ASSERT(child2.computed_size[1] <= 100.f,
             "value should be smaller than max size");

    M_ASSERT(child1.computed_size[0] + child2.computed_size[0] <= 100.f,
             "value of all children should be <= max size");
    M_ASSERT(child1.computed_size[1] + child2.computed_size[1] <= 100.f,
             "value of all children should be <= max size");

    M_ASSERT(child2.computed_size[1] == 99.f,
             "1.f strictness should be enforced");
    M_ASSERT(child1.computed_size[1] <= 200.f,
             "<1.f strictness could have been compromised on");
}

void test_widget_layout() {
    test_empty();
    test_just_root();
    test_single_child();
    test_two_children();
    test_two_children_parent_size();
    test_two_children_parent_size_too_small();
}

int main(void) {
    test_widget_layout();

    return 0;
    //
    startup();
    App::get().run();
    teardown();
    return 0;
}
