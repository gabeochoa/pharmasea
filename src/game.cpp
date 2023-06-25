
// Global Defines

// steam networking uses an "app id" that we dont have
// also the code isnt written yet :)
// TODO: add support for steam connections
#define BUILD_WITHOUT_STEAM

// Enable tracing
// its also defined in:
// - vendor/tracy/TracyClient.cpp
// - app.cpp
// TODO find a way to enable this in the compiler so we dont have to do this
#define ENABLE_TRACING 1
#include "engine/tracy.h"
//

#include "external_include.h"

///
#include "globals.h"
///

#include "engine.h"
//
#include "layers/aboutlayer.h"
#include "layers/fpslayer.h"
#include "layers/gamedebuglayer.h"
#include "layers/gamelayer.h"
#include "layers/menulayer.h"
#include "layers/menustatelayer.h"
#include "layers/networklayer.h"
#include "layers/pauselayer.h"
#include "layers/playerinfolayer.h"
#include "layers/settingslayer.h"
#include "layers/streamersafelayer.h"
#include "layers/toastlayer.h"
#include "layers/versionlayer.h"

//
// This one should be last
#include "./tests/all_tests.h"

void startup() {
    // TODO need to test on lower framerates, there seems to be issues with
    // network initlization

    // Force the app to be created.
    // This unlocks GPU access so we can load textures
    App::create(AppSettings{
        //
        120,  // TODO setting this to any other number seems ot break the camera
              // entity global?
        //
        WIN_W(),
        WIN_H(),
        //
        "PharmaSea",
        //
        raylib::LOG_WARNING,
    });

    // -------- Its unlikely anything should go above this line ----- //

    // Doesnt strictly need to be before preload but just to be safe
    Files::create(FilesConfig{
        GAME_FOLDER,
        SETTINGS_FILE_NAME,
    });

    // Load save file so username is ready for when network starts
    // Load before preload incase we need to read file names or fonts from files
    Settings::get().load_save_file();

    // Has to happen after init window due
    // to font requirements
    Preload::create();

    // Note: there was an issue where the master volume wasnt being respected
    // until you open the settings page.
    //
    // Having this line here fixes that
    Settings::get().refresh_settings();

    MenuState::get().reset();
    GameState::get().reset();

    Layer* layers[] = {
        //
        new FPSLayer(),
        new StreamerSafeLayer(),
        new MenuStateLayer(),
        new PlayerInfoLayer(),
        new VersionLayer(),
        new ToastLayer(),
        //
        new PauseLayer(),
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
    tests::run_all();
    std::cout << "All tests ran " << std::endl;

    startup();
    defer(Settings::get().write_save_file());
    App::get().run();
    return 0;
}
