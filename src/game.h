
// Global Defines

// steam networking uses an "app id" that we dont have
// also the code isnt written yet :)
// TODO :IMPACT: add support for steam connections
#include "engine/log.h"
#define BUILD_WITHOUT_STEAM

// Enable tracing
// its also defined in:
// - vendor/tracy/TracyClient.cpp
// - app.cpp
// TODO :INFRA: find a way to enable this in the compiler so we dont have to do
// this #define ENABLE_TRACING 1 #include "engine/tracy.h"
#define ZoneScoped

#define ENABLE_DEV_FLAGS 1

#include "external_include.h"
#if ENABLE_DEV_FLAGS
#include <argh.h>
#endif

///
#include "globals.h"
///

#include "engine.h"
//
#include "layers/aboutlayer.h"
#include "layers/fpslayer.h"
#include "layers/gamedebuglayer.h"
#include "layers/gamelayer.h"
#include "layers/handlayer.h"
#include "layers/menulayer.h"
#include "layers/menustatelayer.h"
#include "layers/networklayer.h"
#include "layers/pauselayer.h"
#include "layers/seedmanagerlayer.h"
#include "layers/settingslayer.h"
#include "layers/streamersafelayer.h"
#include "layers/toastlayer.h"
#include "layers/uitestlayer.h"
#include "layers/versionlayer.h"

extern ui::UITheme DEFAULT_THEME;

//
// This one should be last
#include "./tests/all_tests.h"

void startup() {
    // TODO :INFRA: need to test on lower framerates, there seems to be issues
    // with network initlization

    // Force the app to be created.
    // This unlocks GPU access so we can load textures
    App::create(AppSettings{
        //
        120,  // TODO :INFRA: setting this to any other number seems ot break
              // the camera
              // entity global?
        //
        WIN_W(),
        WIN_H(),
        //
        strings::GAME_NAME,
        //
        raylib::LOG_WARNING,
    });

    // -------- Its unlikely anything should go above this line ----- //

    // Doesnt strictly need to be before preload but just to be safe
    Files::create(FilesConfig{
        strings::GAME_FOLDER,
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

    // What i realized is that somehow every time i write a test
    // it fixes the component bug im investigating
    //
    // And so im thinking wait maybe theres some bug where if the host has a
    // component but the client doesnt have it registered yet, or the order is
    // different it doesnt deserialize the data correctly (size / color w/e)
    //
    // okay well i can fix that by just forcing the order to remain the same by
    // creating an entity that just adds all the components in order
    //
    // and thats this
    register_all_components();

    MenuState::get().reset();
    GameState::get().reset();

    if (ENABLE_UI_TEST) {
        MenuState::get().set(menu::State::UI);
        Layer* layers[] = {
            //
            new UITestLayer(),
        };
        for (auto layer : layers) App::get().pushLayer(layer);
        return;
    }

    Layer* layers[] = {
        //
        new FPSLayer(),
        new StreamerSafeLayer(),
        new MenuStateLayer(),
        new VersionLayer(),
        new ToastLayer(),
        //
        new HandLayer(),
        //
        new PauseLayer(),
        //
        new GameDebugLayer(),
        new SeedManagerLayer(),
        //
        new NetworkLayer(),
        new GameLayer(),
        new AboutLayer(),
        new SettingsLayer(),
        new MenuLayer(),
    };
    for (auto layer : layers) App::get().pushLayer(layer);
}

int setup_multiplayer_test(bool is_host = false) {
    network::mp_test::enabled = true;
    network::mp_test::is_host = is_host;
    MenuState::get().set(menu::State::Network);
    return 0;
}

void process_dev_flags(char* argv[]) {
#if ENABLE_DEV_FLAGS
    argh::parser cmdl(argv);

    network::ENABLE_REMOTE_IP = true;

    if (cmdl[{"--gabe", "-g"}]) {
        ENABLE_MODELS = true;
        ENABLE_SOUND = false;
        network::ENABLE_REMOTE_IP = false;
        // ENABLE_UI_TEST = true;
        return;
    }

    if (cmdl[{"--tests-only", "-t"}]) {
        TESTS_ONLY = true;
        ENABLE_MODELS = false;
        ENABLE_SOUND = false;
        network::ENABLE_REMOTE_IP = false;
    }

    if (cmdl[{"--disable-all", "-d"}]) {
        ENABLE_MODELS = false;
        ENABLE_SOUND = false;
        network::ENABLE_REMOTE_IP = false;
    }

    if (cmdl[{"--models", "-m"}]) {
        ENABLE_MODELS = true;
    }

    if (cmdl[{"--disable-models", "-M"}]) {
        ENABLE_MODELS = true;
    }
    if (cmdl[{"--sound", "-s"}]) {
        ENABLE_SOUND = true;
    }

    if (cmdl[{"--disable-sound", "-S"}]) {
        ENABLE_SOUND = false;
    }

#endif
}

int main(int argc, char* argv[]);
