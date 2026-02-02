
#include "game.h"

#include "cli_args.h"
#include "engine/assert.h"
#include "engine/input_helper.h"

// Layered input system
#include "game_actions.h"
#include "input_mapping_persistence.h"
#include "input_mapping_setup.h"

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

// Configuration for init_app_core()
struct AppInitConfig {
    const char* window_title = strings::GAME_NAME;
    bool init_network = true;
    bool register_menu_state_callback = true;
    bool init_sound = true;
};

// Core initialization shared between startup() and special modes (e.g.,
// MAP_VIEWER)
void init_app_core(const AppInitConfig& config = {}) {
    // Force the app to be created.
    // This unlocks GPU access so we can load textures
    App::create(App::AppSettings{
        240,
        WIN_W(),
        WIN_H(),
        config.window_title,
        raylib::LOG_ERROR,
    });

    Entity& singleton_entity = ::EntityHelper::createPermanentEntity();

    // Initialize window_manager singleton components for resolution tracking
    {
        auto current_rez =
            afterhours::window_manager::fetch_current_resolution();
        auto available_rez =
            afterhours::window_manager::fetch_available_resolutions();
        afterhours::window_manager::add_singleton_components(
            singleton_entity, current_rez, 240, available_rez);
    }

    // Initialize sound system
    if (config.init_sound) {
        afterhours::sound_system::add_singleton_components(singleton_entity);
    }

    // Initialize layered input system for polling-based input
    {
        afterhours::layered_input<menu::State>::add_singleton_components(
            singleton_entity, game::create_default_layered_mapping(),
            game::get_initial_input_layer());

        // Register callback to switch input layer when menu state changes
        if (config.register_menu_state_callback) {
            MenuState::get().register_on_change(
                [](menu::State new_state, menu::State) {
                    auto* mapper = afterhours::EntityHelper::get_singleton_cmp<
                        afterhours::ProvidesLayeredInputMapping<menu::State>>();
                    if (mapper) {
                        mapper->set_active_layer(new_state);
                    }
                });
        }
    }

    // Initialize file system
    Files::create(FilesConfig{
        strings::GAME_FOLDER,
        SETTINGS_FILE_NAME,
    });

    // Load settings
    Settings::get().load_save_file();
    Settings::get().refresh_settings();

    // Load assets (requires init window for fonts)
    Preload::create();

    // Register all components for serialization
    register_all_components();

    // Initialize network if requested
    if (config.init_network) {
        network::init_connections();

        if (!network::LOCAL_ONLY) {
            MenuState::get().reset();
            GameState::get().reset();
        }
    }

    // Initialize input helper after state managers are ready
    input_helper::init();
}

void startup() {
    // TODO :INFRA: need to test on lower framerates, there seems to be issues
    // with network initlization

    // Initialize simulated input before core init
    simulated_input::init();

    // Core initialization (app, window, input, files, settings, preload,
    // network)
    init_app_core();

    // Determine whether to show raylib intro based on save file existence
    fs::path settings_path = Files::get().settings_filepath();
    bool save_file_exists = fs::exists(settings_path);
    SHOW_RAYLIB_INTRO = SHOW_INTRO || !save_file_exists;

    log_info(
        "intro: path={}, exists={}, show_intro_flag={}, show_raylib={}",
        settings_path.string(), save_file_exists, SHOW_INTRO, SHOW_RAYLIB_INTRO);
    std::cout << "[intro] path=" << settings_path
              << " exists=" << save_file_exists
              << " show_intro_flag=" << SHOW_INTRO
              << " show_raylib=" << SHOW_RAYLIB_INTRO << std::endl;
    log_info("intro: game_folder={}", Files::get().game_folder().string());

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

    if (MAP_VIEWER) {
        std::string seed =
            MAP_VIEWER_SEED.empty() ? "default_seed" : MAP_VIEWER_SEED;

        log_info("Executable Path: {}", fs::current_path());
        log_info("Canon Path: {}", fs::canonical(fs::current_path()));

        // Use shared core initialization with map viewer config
        init_app_core({
            .window_title = "PharmaSea Map Viewer",
            .init_network = false,
            .register_menu_state_callback = false,
            .init_sound = false,
        });

        App::get().loadLayers({{
            new FPSLayer(),
            new MapViewerLayer(seed),
        }});

        try {
            App::get().run();
        } catch (const std::exception& e) {
            std::cout << "Map viewer exception: " << e.what() << std::endl;
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
