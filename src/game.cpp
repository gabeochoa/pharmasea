
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

int main(void) {
    using namespace ui;
    Widget root;
    root.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Pixels,
        .value= WIN_W,
        .strictness=  1.f
    };
    root.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Pixels,
        .value= WIN_H,
        .strictness=  1.f
    };

    Widget child1;
    child1.size_expected[0] = SizeExpectation{
        .mode = SizeMode::Percent,
        .value= 0.5f,
        .strictness=  1.f
    };
    child1.size_expected[1] = SizeExpectation{
        .mode = SizeMode::Percent,
        .value= 0.5f,
        .strictness=  1.f
    };
    root.first = &child1;
    root.last = &child1;
    root.next = &child1;

    child1.parent = &root;

    std::cout << "before\n" << root << std::endl;

    process_widget(&root);

    std::cout << "after\n" << root << std::endl;
    std::cout << child1 << std::endl;
    

    return 0;
    //
    startup();
    App::get().run();
    teardown();
    return 0;
}
