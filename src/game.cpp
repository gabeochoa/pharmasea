
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
#include "networklayer.h"
#include "settingslayer.h"
#include "versionlayer.h"
//
// This one should be last
#include "./tests/all_tests.h"

void startup() {
    SetTargetFPS(120);
    // Disable all that startup logging
    SetTraceLogLevel(LOG_WARNING);
    // Force the app to be created.
    // This unlocks GPU access so we can load textures
    App::get();

    // -------- Its unlikely anything should go above this line ----- //

    tests::run_all();
    std::cout << "All tests ran successfully" << std::endl;

    Menu::get().state = Menu::State::Root;
    // Menu::get().state = Menu::State::About;
    // Menu::get().state = Menu::State::Game;

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

    NetworkLayer* networklayer = new NetworkLayer();
    App::get().pushLayer(networklayer);

    Settings::get().load_save_file();
}

void teardown() {
#ifdef WRITE_FILES
    Settings::get().write_save_file();
#endif
}

int main(void) {
    startup();
    App::get().run();
    teardown();
    return 0;
}
