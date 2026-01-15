#include "simulated_input.h"

#include <filesystem>
#include <optional>
#include <string>

#include "../../globals.h"
#include "../app.h"
#include "../files.h"
#include "../log.h"
#include "../statemanager.h"
#include "bypass_helper.h"

namespace fs = std::filesystem;

namespace simulated_input {

namespace {
constexpr const char* kRecordedInputsDirName = "recorded_inputs";
constexpr const char* kDefaultRecordName = "recorded_input.txt";
constexpr const char* kDefaultBypassMenuReplay = "bypass_menu.txt";
constexpr const char* kDefaultBypassRoundReplay = "bypass_round1.txt";

fs::path recorded_dir() { return fs::current_path() / kRecordedInputsDirName; }

void ensure_dir() { fs::create_directories(recorded_dir()); }

void register_state_recorders() {
    static bool registered = false;
    if (registered) return;
    MenuState::get().register_on_change([](menu::State ns, menu::State) {
        input_recorder::record_state_change("menu_state", static_cast<int>(ns));
    });
    GameState::get().register_on_change([](game::State ns, game::State) {
        input_recorder::record_state_change("game_state", static_cast<int>(ns));
    });
    registered = true;
}

void start_record_if_requested() {
    if (!RECORD_INPUTS) return;
    fs::path record_path = recorded_dir() / kDefaultRecordName;
    input_recorder::enable(record_path);
    log_info("Recording input to {}", record_path.string());
}

void start_replay_for_bypass_menu_or_replay_flag() {
    log_info("SimInput: start_replay check enabled={} name='{}' bypass={}",
             REPLAY_ENABLED, REPLAY_NAME, BYPASS_MENU);
    std::optional<fs::path> replay_path;
    if (REPLAY_ENABLED && !REPLAY_NAME.empty()) {
        replay_path = recorded_dir() / (REPLAY_NAME + ".txt");
        log_info("SimInput: replay flag set, target={}", replay_path->string());
    } else if (BYPASS_MENU) {
        replay_path = recorded_dir() / kDefaultBypassMenuReplay;
        log_info("SimInput: bypass-menu replay target={}",
                 replay_path->string());
    }

    if (!replay_path.has_value()) return;

    if (!fs::exists(*replay_path)) {
        log_error("Replay file missing: {}", replay_path->string());
        return;
    }
    input_replay::start(*replay_path);
}
}  // namespace

void init() {
    ensure_dir();
    register_state_recorders();
    start_record_if_requested();
    start_replay_for_bypass_menu_or_replay_flag();
}

void update(float dt) {
    if (input_recorder::is_enabled()) {
        input_recorder::update_poll();
    }
    input_replay::update(dt);
    bypass_helper::inject_clicks_for_bypass(dt);
    input_injector::update_key_hold(dt);
}

void stop() {
    input_replay::stop();
    input_recorder::shutdown();
}

void start_round_replay() {
    fs::path round_path = recorded_dir() / kDefaultBypassRoundReplay;
    if (!fs::exists(round_path)) {
        log_error("Replay file missing: {}", round_path.string());
        return;
    }
    input_replay::start(round_path);
}

}  // namespace simulated_input
