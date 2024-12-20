

#include "game.h"

#include "engine/assert.h"
#include "engine/random_engine.h"
#include "engine/ui/svg.h"
#include "map_generation.h"

// TODO removing this include would speed up
// compilation times of this file by probably 1.6 seconds
// (this is due to all the template code thats in network/shared
#include "network/network.h"

// TODO cleaning bots place some shiny on the floor so its harder to get dirtyu
// TODO fix bug where "place ghost" green box kept showing
// TODO conveyerbelt speed
// TODO tutorial should only apply when host has it on
// TODO save tutorial in save file
// TODO affect tip based on how much vomit is around?
// => how can we communicate this to the player
// TODO can you check if the tutorial buttons work for gamepad, its not
// switching
// TODO check polymorphic_components script should check entity_makers too
// TODO when the resolution is larger than the monitor, pop up something to
// suggest changing it, if the diff is too large, then itll be impossible to
// change the resolution
// TODO move network polling to its own thread and use a message queue
// TODO ffwd progress bar needs to hide when you are not near
// TODO turn back on automatic analysis
// https://sonarcloud.io/project/analysis_method?id=gabeochoa_pharmasea

// branch
// - TODO highlight seems to be one off or less reliable but the pick up is
// still correct
// - TODO should store teleport items or do you walk them over
// - TODO update reason text to say "customers wont like it if you dont "
// - TODO its a little awkward when the day ends and customers should pay but
// they leave
// - TODO handtruck reach is really bad, should just make it pretend to be
// normal grab/drop, for now you dont need it
// - TODO settings button "keyboard" is weird
// - TODO Rotate should be disabled when recipe page is open
// - TODO ffwding will basically tell everyone to go home even if they had a
// drink. you probably want to allow people to pay even when closed? maybe just
// spawn everyone in the first half of the day, and only allow you to ffwd until
// everyone is spawned and then stop
//
// mop buddy in store loses buddy
//
// - store should give one free reroll per day
//  -> should you just walk out of the store and be charged? amazon go style?
// - add post office for free items from new recipies
// -> mailbox upgrade which gives a closer place to pick up items
// - school should give new recipies
// - recycle / landfill for selling&destroying furniture you dont need
// - add tutorial for using handtruck
// - add upgraded handtruck for easier movement
//
// open questions
// - are there items you should not be able to move during the day and how will
// the player know
// - how bad is it if other players can see items spawn in?
//  -> maybe the upgrade just "mails" the items to the spawn area (eventually
//  can add a parachute animation or something )

namespace network {
long long total_ping = 0;
long long there_ping = 0;
long long return_ping = 0;
}  // namespace network

void startup() {
    // TODO :INFRA: need to test on lower framerates, there seems to be issues
    // with network initlization

    // Force the app to be created.
    // This unlocks GPU access so we can load textures
    App::create(App::AppSettings{
        //
        240,
        //
        WIN_W(), WIN_H(),
        //
        strings::GAME_NAME,
        //
        raylib::LOG_ERROR,
        //
    });

    // -------- Its unlikely anything should go above this line ----- //

    // Doesnt strictly need to be before preload but just to be safe
    Files::create(FilesConfig{
        strings::GAME_FOLDER,
        SETTINGS_FILE_NAME,
    });

    // Load save file so username is ready for when network starts
    // Load before preload incase we need to read file names or fonts from
    // files
    Settings::get().load_save_file();

    // Has to happen after init window due
    // to font requirements
    Preload::create();

    // Note: there was an issue where the master volume wasnt being
    // respected until you open the settings page.
    //
    // Having this line here fixes that
    Settings::get().refresh_settings();

    // What i realized is that somehow every time i write a test
    // it fixes the component bug im investigating
    //
    // And so im thinking wait maybe theres some bug where if the host has a
    // component but the client doesnt have it registered yet, or the order
    // is different it doesnt deserialize the data correctly (size / color
    // w/e)
    //
    // okay well i can fix that by just forcing the order to remain the same
    // by creating an entity that just adds all the components in order
    //
    // and thats this
    register_all_components();

    network::Info::init_connections();

    MenuState::get().reset();
    GameState::get().reset();

    App::get().loadLayers({{
        //
        new FPSLayer(),
        new StreamerSafeLayer(),
        new VersionLayer(),
        //
        new HandLayer(),
        new ToastLayer(),
        new SettingsLayer(),
        //
        new PauseLayer(),
        //
        //
        new DebugSettingsLayer(),
        new RecipeBookLayer(),
        new RoundEndReasonLayer(),
        new SeedManagerLayer(),
        new GameDebugLayer(),  // putting below seed manager since
                               // typing 'o' switches modes
                               //
        new NetworkLayer(),
        new GameLayer(),
        new AboutLayer(),
        new MenuLayer(),
    }});
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

#include "engine/util.h"

int main(int, char* argv[]) {
    process_dev_flags(argv);

#if ENABLE_TESTS
    tests::run_all();

    if (TESTS_ONLY) {
        log_info("All tests ran ");
        return 0;
    }

#endif

    log_info("Executable Path: {}", fs::current_path());
    log_info("Canon Path: {}", fs::canonical(fs::current_path()));

    startup();

    // wfc::WaveCollapse wc(static_cast<unsigned
    // int>(hashString("WVZ_ORYYVAV"))); wc.run(); wc.get_lines(); return 0;
    //
    try {
        App::get().run();
    } catch (const std::exception& e) {
        std::cout << "App run exception: " << e.what() << std::endl;
    }
    return 0;
}
