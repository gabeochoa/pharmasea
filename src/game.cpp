
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
#include "fpslayer.h"
#include "gamedebuglayer.h"
#include "gamelayer.h"
#include "menulayer.h"
#include "network/networklayer.h"
#include "pauselayer.h"
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

    // make player
    std::shared_ptr<Player> player;
    player.reset(new Player());
    GLOBALS.set("player", player.get());
    EntityHelper::addEntity(player);

    // We need to be able to distinguish between
    // - Layer that is always running (fpslayer)
    // - layer that is just an overlay and can be immediately alloc/dealloc
    // (pause menu)
    //
    // // not supported use a global
    // - Layer that is not always running but needs to keep memory around
    Layer* always_on_layers[] = {
        new FPSLayer(),
        new GameDebugLayer(),
        new NetworkLayer(),
        new VersionLayer(),
        //
        new MenuLayer(),
    };
    for (auto layer : always_on_layers) App::get().pushLayer(layer);

    Settings::get().load_save_file();
}

int main(void) {
    startup();
    defer(Settings::get().write_save_file());
    App::get().run();
    return 0;
}
