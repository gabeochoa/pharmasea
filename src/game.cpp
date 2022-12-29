
// Global Defines

// steam networking uses an "app id" that we dont have
// also the code isnt written yet :)
// TODO: add support for steam connections
#define BUILD_WITHOUT_STEAM

#include "external_include.h"

///
#include "globals.h"
///

#include "app.h"
#include "engine/defer.h"
#include "menu.h"
#include "settings.h"
//
#include "aboutlayer.h"
#include "fpslayer.h"
#include "gamedebuglayer.h"
#include "gamelayer.h"
#include "menulayer.h"
#include "menustatelayer.h"
#include "networklayer.h"
#include "pauselayer.h"
#include "settingslayer.h"
#include "streamersafelayer.h"
#include "versionlayer.h"
//
// This one should be last
#include "./tests/all_tests.h"

void startup() {
    // Force the app to be created.
    // This unlocks GPU access so we can load textures
    App::get_and_create(AppSettings{
        //
        120,
        //
        WIN_W(),
        WIN_H(),
        //
        "PharmaSea",
        //
        []() {},
        //
        LOG_WARNING,
    });

    // -------- Its unlikely anything should go above this line ----- //

    // Has to happen after init window due
    // to font requirements
    Preload::get();

    tests::run_all();
    std::cout << "All tests ran successfully" << std::endl;

    // Load save file so username is ready for when network starts
    Settings::get().load_save_file();

    Menu::get().set(Menu::State::Root);

    Layer* layers[] = {
        //
        new FPSLayer(),
        new StreamerSafeLayer(),
        new MenuStateLayer(),
        new VersionLayer(),
        //
        new PauseLayer(),
        new PausePlanningLayer(),
        //
        new NetworkLayer(),
        new GameLayer(),
        new GameDebugLayer(),
        new AboutLayer(),
        new SettingsLayer(),
        new MenuLayer(),
    };
    for (auto layer : layers) App::get().pushLayer(layer);
}

int main(void) {
    startup();
    defer(Settings::get().write_save_file());
    App::get().run();
    return 0;
}
