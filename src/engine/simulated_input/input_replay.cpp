#include <chrono>
#include <fstream>
#include <string>
#include <vector>

#include "../app.h"
#include "../event.h"
#include "../log.h"
#include "../statemanager.h"
#include "replay_validation.h"
#include "simulated_input.h"

namespace input_replay {

namespace {
struct ReplayEvent {
    long long timestamp_ms = 0;
    enum class Type { Key, Mouse, MouseMove, State } type = Type::Key;
    int code = 0;
    std::string action;  // "down", "up", "x_y" for move, or state label
};

struct ReplayState {
    bool active = false;
    std::vector<ReplayEvent> events;
    size_t idx = 0;
    float elapsed_ms = 0.0f;
    std::chrono::steady_clock::time_point start_time{};
};

static ReplayState state;
static std::chrono::steady_clock::time_point last_now{};

ReplayEvent::Type type_from_string(const std::string& s) {
    if (s == "menu_state" || s == "game_state") return ReplayEvent::Type::State;
    if (s == "mouse") return ReplayEvent::Type::Mouse;
    if (s == "mouse_move") return ReplayEvent::Type::MouseMove;
    return ReplayEvent::Type::Key;
}

const char* type_to_string(ReplayEvent::Type t) {
    switch (t) {
        case ReplayEvent::Type::Key:
            return "key";
        case ReplayEvent::Type::Mouse:
            return "mouse";
        case ReplayEvent::Type::MouseMove:
            return "mouse_move";
        case ReplayEvent::Type::State:
            return "state";
    }
    return "unknown";
}

void dispatch(const ReplayEvent& ev) {
    if (ev.type == ReplayEvent::Type::State) {
        if (ev.action == "menu_state") {
            menu::State current = MenuState::get().read();
            int current_int = static_cast<int>(current);
            if (current_int != ev.code) {
                log_warn("replay: menu_state mismatch recorded={} current={}",
                         ev.code, current_int);
            } else {
                log_info("replay: menu_state match {}", current_int);
            }
        } else if (ev.action == "game_state") {
            game::State current = GameState::get().read();
            int current_int = static_cast<int>(current);
            if (current_int != ev.code) {
                log_warn("replay: game_state mismatch recorded={} current={}",
                         ev.code, current_int);
            } else {
                log_info("replay: game_state match {}", current_int);
            }
        } else {
            log_warn("replay: unknown state label '{}' val={}", ev.action,
                     ev.code);
        }
        return;
    }

    if (ev.type == ReplayEvent::Type::Key) {
        if (ev.action == "down") {
            KeyPressedEvent e(ev.code, 0);
            App::get().processEvent(e);
            input_injector::set_key_down(ev.code);
            log_info("replay: key {} down", ev.code);
        } else if (ev.action == "up") {
            input_injector::set_key_up(ev.code);
            log_info("replay: key {} up (no explicit event)", ev.code);
        }
        return;
    }

    if (ev.type == ReplayEvent::Type::Mouse) {
        int button_code = ev.code;
        if (ev.action == "down") {
            Mouse::MouseButtonDownEvent e(button_code);
            App::get().processEvent(e);
            Mouse::MouseButtonPressedEvent e2(button_code);
            App::get().processEvent(e2);
            log_info("replay: mouse {} down", button_code);
        } else if (ev.action == "up") {
            Mouse::MouseButtonUpEvent e(button_code);
            App::get().processEvent(e);
            Mouse::MouseButtonReleasedEvent e2(button_code);
            App::get().processEvent(e2);
            log_info("replay: mouse {} up", button_code);
        }
    }

    if (ev.type == ReplayEvent::Type::MouseMove) {
        // Parse "x_y"
        const std::string& coord = ev.action;  // stored in action for move
        size_t sep = coord.find('_');
        if (sep != std::string::npos) {
            try {
                float x = std::stof(coord.substr(0, sep));
                float y = std::stof(coord.substr(sep + 1));
                raylib::SetMousePosition(static_cast<int>(x),
                                         static_cast<int>(y));
                ui::focus::mouse_info.pos = {x, y};
                Mouse::MouseMovedEvent move_event(x, y);
                App::get().processEvent(move_event);
                log_info("replay: mouse move {} {}", x, y);
            } catch (...) {
                log_error("replay: failed to parse mouse move '{}'", coord);
            }
        }
    }
}
}  // namespace

void start(const std::filesystem::path& path) {
    state = ReplayState{};
    replay_validation::start_replay(path.stem().string());
    std::ifstream file(path);
    if (!file.is_open()) {
        log_error("Input replay: failed to open {}", path.string());
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        size_t p1 = line.find(',');
        size_t p2 = line.find(',', p1 + 1);
        size_t p3 = line.find(',', p2 + 1);
        if (p1 == std::string::npos || p2 == std::string::npos ||
            p3 == std::string::npos) {
            continue;
        }
        ReplayEvent ev;
        try {
            ev.timestamp_ms = std::stoll(line.substr(0, p1));
        } catch (...) {
            continue;
        }
        ev.type = type_from_string(line.substr(p1 + 1, p2 - p1 - 1));
        if (ev.type == ReplayEvent::Type::MouseMove) {
            ev.code = 0;
            ev.action = line.substr(p2 + 1, p3 - p2 - 1);  // x_y string
        } else {
            try {
                ev.code = std::stoi(line.substr(p2 + 1, p3 - p2 - 1));
            } catch (...) {
                continue;
            }
            // For state rows, keep the label in action (e.g., menu_state) so we
            // can validate correctly; the recorder writes the label as the
            // second column and "state" as the last column.
            if (ev.type == ReplayEvent::Type::State) {
                ev.action = line.substr(p1 + 1, p2 - p1 - 1);
            } else {
                ev.action = line.substr(p3 + 1);
            }
        }
        state.events.push_back(ev);
    }

    if (state.events.empty()) {
        log_error("Input replay: no events loaded from {}", path.string());
        return;
    }

    state.active = true;
    state.idx = 0;
    state.elapsed_ms = 0.0f;
    state.start_time = {};
    log_info("Input replay: loaded {} events from {}", state.events.size(),
             path.string());
}

void stop() { state = ReplayState{}; }

bool is_active() { return state.active; }

void update(float) {
    if (!state.active) return;

    auto now = std::chrono::steady_clock::now();
    if (last_now == std::chrono::steady_clock::time_point{}) {
        last_now = now;
    }

    if (state.start_time == std::chrono::steady_clock::time_point{}) {
        long long first_ts =
            state.events.empty() ? 0 : state.events.front().timestamp_ms;
        if (first_ts < 0) first_ts = 0;
        state.start_time = now - std::chrono::milliseconds(first_ts);
        state.elapsed_ms = 0.0f;
        last_now = now;
        log_info("replay: start aligned with first_ts={}ms", first_ts);
    }

    state.elapsed_ms = static_cast<float>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now -
                                                              state.start_time)
            .count());

    last_now = now;

    static float last_debug_ms = -500.0f;
    if (state.elapsed_ms - last_debug_ms >= 500.0f) {
        long long next_ts = (state.idx < state.events.size())
                                ? state.events[state.idx].timestamp_ms
                                : -1;
        log_info("replay: tick elapsed={:.1f}ms idx={} next_ts={}",
                 state.elapsed_ms, state.idx, next_ts);
        last_debug_ms = state.elapsed_ms;
    }

    if (state.idx < state.events.size() &&
        state.elapsed_ms + 0.01f >= state.events[state.idx].timestamp_ms) {
        const ReplayEvent& ev = state.events[state.idx];
        log_info(
            "replay: dispatch idx={} at {:.1f}ms type={} code={} action={}",
            state.idx, state.elapsed_ms, type_to_string(ev.type), ev.code,
            ev.action);
        dispatch(state.events[state.idx]);
        state.idx++;
    }

    if (state.idx >= state.events.size()) {
        state.active = false;
        replay_validation::end_replay();
        if (EXIT_ON_BYPASS_COMPLETE) {
            log_info(
                "replay: completed all events; exit-on-bypass-complete set, "
                "closing app");
            App::get().close();
        }
    }
}

}  // namespace input_replay
