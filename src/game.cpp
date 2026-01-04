

#include "game.h"

#include "engine/assert.h"

// Global flag storage for re-application after settings load
bool disable_models_flag = false;
#include "engine/random_engine.h"
#include "engine/simulated_input/simulated_input.h"
#include "engine/ui/svg.h"
#include "map_generation/map_generation.h"
#include "map_generation/pipeline.h"
#include "map_generation/wave_collapse.h"

// TODO removing this include would speed up
// compilation times of this file by probably 1.6 seconds
// (this is due to all the template code thats in network/shared
#include "network/api.h"

#if !defined(NDEBUG)
#include "backward/backward.hpp"
#endif

#if !defined(NDEBUG)
#include "backward/backward.hpp"
#endif

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

// Define BYPASS_MENU (declared as extern in globals.h)
bool BYPASS_MENU = false;
int BYPASS_ROUNDS = 0;
bool EXIT_ON_BYPASS_COMPLETE = false;
bool RECORD_INPUTS = false;
std::string REPLAY_NAME = "";
bool REPLAY_ENABLED = false;
bool REPLAY_VALIDATE = false;
bool SHOW_INTRO = false;
bool SHOW_RAYLIB_INTRO = false;
bool TEST_MAP_GENERATION = false;
bool GENERATE_MAP = false;
std::string GENERATE_MAP_SEED = "";
std::string LOAD_SAVE_TARGET = "";
bool LOAD_SAVE_ENABLED = false;

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

    simulated_input::init();

    // Load save file so username is ready for when network starts
    // Load before preload incase we need to read file names or fonts from
    // files
    fs::path settings_path = Files::get().settings_filepath();
    bool save_file_exists = fs::exists(settings_path);
    bool has_save = Settings::get().load_save_file();

    // Skip raylib splash whenever a save file exists unless explicitly forced.
    SHOW_RAYLIB_INTRO = SHOW_INTRO || !save_file_exists;

    log_info(
        "intro: path={}, exists={}, has_save={}, show_intro_flag={}, "
        "show_raylib={}",
        settings_path.string(), save_file_exists, has_save, SHOW_INTRO,
        SHOW_RAYLIB_INTRO);
    std::cout << "[intro] path=" << settings_path
              << " exists=" << save_file_exists << " has_save=" << has_save
              << " show_intro_flag=" << SHOW_INTRO
              << " show_raylib=" << SHOW_RAYLIB_INTRO << std::endl;
    log_info("intro: game_folder={}", Files::get().game_folder().string());

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

    network::init_connections();

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

void process_dev_flags(int argc, char* argv[]) {
#if ENABLE_DEV_FLAGS
    // Early reject unknown flags so we don't run with unintended args.
    auto is_known_flag = [](const std::string& arg) {
        static const std::set<std::string> no_value = {
            "--gabe",
            "-g",
            "--tests-only",
            "-t",
            "--disable-all",
            "-d",
            "--models",
            "-m",
            "--disable-models",
            "-M",
            "--sound",
            "-s",
            "--disable-sound",
            "-S",
            "--bypass-menu",
            "--local",
            "--exit-on-bypass-complete",
            "--record-input",
            "--intro",
            "--test_map_generation",
            "--replay-validate"};
        static const std::set<std::string> with_value = {
            "--replay",
            "--bypass-rounds",
            "--generate-map",
            "--load-save",
        };

        // Accept --flag=value form for value flags.
        if (arg.rfind("--", 0) == 0) {
            size_t eq = arg.find('=');
            if (eq != std::string::npos) {
                std::string head = arg.substr(0, eq);
                return with_value.count(head) > 0;
            }
        }
        if (no_value.count(arg) > 0) return true;
        if (with_value.count(arg) > 0) return true;
        return false;
    };

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i] ? argv[i] : "";
        if (arg.empty() || arg[0] != '-') continue;
        if (is_known_flag(arg)) {
            // If this flag expects a value, skip the next token.
            if ((arg == "--replay" || arg == "--bypass-rounds" ||
                 arg == "--generate-map" || arg == "--load-save") &&
                i + 1 < argc) {
                ++i;
            }
            continue;
        }
        log_error("Unknown flag '{}'; exiting.", arg);
        std::exit(1);
    }

    argh::parser cmdl(argc, argv);
    network::ENABLE_REMOTE_IP = true;
    network::LOCAL_ONLY = cmdl[{"--local"}];

    // Store flag values for re-application after settings load
    disable_models_flag = cmdl[{"--disable-models", "-M"}];

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

    if (disable_models_flag) {
        ENABLE_MODELS = false;
    }
    if (cmdl[{"--sound", "-s"}]) {
        ENABLE_SOUND = true;
    }

    if (cmdl[{"--disable-sound", "-S"}]) {
        ENABLE_SOUND = false;
    }

    if (cmdl[{"--bypass-menu"}]) {
        BYPASS_MENU = true;
        log_info(
            "Bypass: --bypass-menu flag detected, BYPASS_MENU set to true");
    }

    if (cmdl({"--replay"})) {
        std::string name;
        cmdl({"--replay"}) >> name;
        if (!name.empty()) {
            REPLAY_NAME = name;
            REPLAY_ENABLED = true;
            log_info("Replay: will load recorded_inputs/{}.txt", REPLAY_NAME);
        }
    }

    if (cmdl({"--load-save"})) {
        std::string target;
        cmdl({"--load-save"}) >> target;
        if (!target.empty()) {
            LOAD_SAVE_TARGET = target;
            LOAD_SAVE_ENABLED = true;
            log_info("LoadSave: requested '{}'", LOAD_SAVE_TARGET);
        }
    }

    // Fallback manual argv scan in case argh misses parameters
    auto try_set_replay = [](const std::string& val) {
        if (val.empty()) return;
        REPLAY_NAME = val;
        REPLAY_ENABLED = true;
        log_info("Replay: parsed via argv fallback '{}'", REPLAY_NAME);
    };
    auto try_set_load_save = [](const std::string& val) {
        if (val.empty()) return;
        LOAD_SAVE_TARGET = val;
        LOAD_SAVE_ENABLED = true;
        log_info("LoadSave: parsed via argv fallback '{}'", LOAD_SAVE_TARGET);
    };
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i] ? argv[i] : "";
        const std::string prefix = "--replay=";
        if (arg.rfind(prefix, 0) == 0) {
            try_set_replay(arg.substr(prefix.size()));
            continue;
        }
        const std::string load_prefix = "--load-save=";
        if (arg.rfind(load_prefix, 0) == 0) {
            try_set_load_save(arg.substr(load_prefix.size()));
            continue;
        }
        if (arg == "--replay" && i + 1 < argc) {
            try_set_replay(argv[i + 1] ? argv[i + 1] : "");
            ++i;  // consume value
        }
        if (arg == "--load-save" && i + 1 < argc) {
            try_set_load_save(argv[i + 1] ? argv[i + 1] : "");
            ++i;  // consume value
        }
    }

    auto try_set_generate_map = [](const std::string& val) {
        if (val.empty()) return;
        GENERATE_MAP = true;
        GENERATE_MAP_SEED = val;
    };
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i] ? argv[i] : "";
        const std::string prefix = "--generate-map=";
        if (arg.rfind(prefix, 0) == 0) {
            try_set_generate_map(arg.substr(prefix.size()));
            continue;
        }
        if (arg == "--generate-map" && i + 1 < argc) {
            try_set_generate_map(argv[i + 1] ? argv[i + 1] : "");
            ++i;
        }
    }

    if (cmdl({"--bypass-rounds"})) {
        int parsed_rounds = 1;
        cmdl({"--bypass-rounds"}) >> parsed_rounds;
        if (parsed_rounds < 1) parsed_rounds = 1;
        BYPASS_ROUNDS = parsed_rounds;
        BYPASS_MENU = true;
        log_info("Bypass: --bypass-rounds set to {}", BYPASS_ROUNDS);
    }

    if (cmdl[{"--exit-on-bypass-complete"}]) {
        EXIT_ON_BYPASS_COMPLETE = true;
        log_info("--exit-on-bypass-complete enabled");
    }

    if (cmdl[{"--replay-validate"}]) {
        REPLAY_VALIDATE = true;
        log_info("--replay-validate enabled");
    }

    if (cmdl[{"--record-input"}]) {
        RECORD_INPUTS = true;
        log_info("--record-input enabled; inputs will be captured");
    }

    log_info("DevFlags: replay_enabled={} replay_name='{}' bypass_menu={}",
             REPLAY_ENABLED, REPLAY_NAME, BYPASS_MENU);

    if (cmdl[{"--intro"}]) {
        SHOW_INTRO = true;
        SHOW_RAYLIB_INTRO = true;
        log_info("--intro flag detected, forcing intro screen");
    }

    if (cmdl[{"--test_map_generation"}]) {
        TEST_MAP_GENERATION = true;
    }

#endif
}

#include "engine/util.h"

int main(int argc, char* argv[]) {
    process_dev_flags(argc, argv);

#if ENABLE_TESTS
    tests::run_all();

    if (TESTS_ONLY) {
        log_info("All tests ran ");
        return 0;
    }

#endif

    if (GENERATE_MAP) {
        const auto archetype_to_string = [](mapgen::BarArchetype archetype) {
            switch (archetype) {
                case mapgen::BarArchetype::OpenHall:
                    return "OpenHall";
                case mapgen::BarArchetype::MultiRoom:
                    return "MultiRoom";
                case mapgen::BarArchetype::BackRoom:
                    return "BackRoom";
                case mapgen::BarArchetype::LoopRing:
                    return "LoopRing";
            }
            return "Unknown";
        };

        if (GENERATE_MAP_SEED.empty()) {
            log_error("--generate-map requires a non-empty seed");
            return 1;
        }

        RandomEngine::set_seed(GENERATE_MAP_SEED);
        mapgen::GenerationContext ctx;
        mapgen::GeneratedAscii out =
            mapgen::generate_ascii(GENERATE_MAP_SEED, ctx);
        std::cout << "[mapgen] seed=" << GENERATE_MAP_SEED
                  << " archetype=" << archetype_to_string(out.archetype)
                  << " rows=" << ctx.rows << " cols=" << ctx.cols << std::endl;
        for (const std::string& line : out.lines) {
            std::cout << line << std::endl;
        }
        return 0;
    }

    if (TEST_MAP_GENERATION) {
        wfc::ensure_map_generation_info_loaded();
        wfc::WaveCollapse wc(
            static_cast<unsigned int>(hashString("WVZ_ORYYVAV")));
        wc.run();
        wc.get_lines();
        return 0;
    }

    log_info("Executable Path: {}", fs::current_path());
    log_info("Canon Path: {}", fs::canonical(fs::current_path()));

    startup();

    try {
        App::get().run();
    } catch (const std::exception& e) {
        std::cout << "App run exception: " << e.what() << std::endl;
#if !defined(NDEBUG)
        backward::StackTrace st;
        st.load_here(64);
        backward::Printer p;
        p.address = true;
        p.object = true;
        p.print(st);
#endif
    }
    return 0;
}
