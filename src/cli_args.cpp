
#include "cli_args.h"

#include <set>
#include <string>

#include "engine/log.h"
#include "globals.h"

#define ENABLE_DEV_FLAGS 1

#if ENABLE_DEV_FLAGS
#include <argh.h>
#endif

// Network ping tracking (used by network layer)
namespace network {
long long total_ping = 0;
long long there_ping = 0;
long long return_ping = 0;
}  // namespace network

// Global flag storage for re-application after settings load
bool disable_models_flag = false;

// Define flags declared as extern in globals.h
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
bool MAP_VIEWER = false;
std::string MAP_VIEWER_SEED = "";

#ifdef AFTER_HOURS_ENABLE_MCP
bool MCP_ENABLED = false;
#endif

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
            "--replay-validate",
            "--map-viewer",
            "--mcp"};
        static const std::set<std::string> with_value = {
            "--replay",    "--bypass-rounds", "--generate-map",
            "--load-save", "--seed",
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
                 arg == "--generate-map" || arg == "--load-save" ||
                 arg == "--seed") &&
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

    if (cmdl[{"--map-viewer"}]) {
        MAP_VIEWER = true;
        ENABLE_SOUND = false;
        log_info("--map-viewer flag detected");
    }

#ifdef AFTER_HOURS_ENABLE_MCP
    if (cmdl[{"--mcp"}]) {
        MCP_ENABLED = true;
        log_info("--mcp flag detected");
    }
#else
    if (cmdl[{"--mcp"}]) {
        log_warn("--mcp flag ignored: build was compiled without MCP support");
    }
#endif

    // Parse --seed=xyz or --seed xyz
    const auto parse_seed = [&](const std::string& val) {
        if (val.empty()) return;
        MAP_VIEWER_SEED = val;
        log_info("--seed set to '{}'", MAP_VIEWER_SEED);
    };
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i] ? argv[i] : "";
        const std::string prefix = "--seed=";
        if (arg.rfind(prefix, 0) == 0) {
            parse_seed(arg.substr(prefix.size()));
            continue;
        }
        if (arg == "--seed" && i + 1 < argc) {
            parse_seed(argv[++i] ? argv[i] : "");
        }
    }

#endif
}
