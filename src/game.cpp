
// Global Defines

// Team networking uses an "app id" that we dont have
// also the code isnt written yet :)
// TODO: add support for steam connections
#define BUILD_WITHOUT_STEAM

#include "external_include.h"

///
#include "globals.h"
///

#include "app.h"
#include "defer.h"
#include "menu.h"
#include "settings.h"
//
#include "aboutlayer.h"
#include "fpslayer.h"
#include "gamedebuglayer.h"
#include "gamelayer.h"
#include "menulayer.h"
#include "menustatelayer.h"
#include "network/networklayer.h"
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

    Layer* layers[] = {
        new FPSLayer(),       new GameLayer(),     new GameDebugLayer(),
        new AboutLayer(),     new SettingsLayer(), new MenuLayer(),
        new MenuStateLayer(), new VersionLayer(),  new NetworkLayer(),
    };
    for (auto layer : layers) App::get().pushLayer(layer);

    Settings::get().load_save_file();
}

int main(void) {
    startup();
    defer(Settings::get().write_save_file());
    App::get().run();
    return 0;
}
