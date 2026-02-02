#include <chrono>
#include <fstream>
#include <string>

#include "../graphics.h"
#include "../log.h"
#include "simulated_input.h"

namespace input_recorder {

namespace {
struct RecorderState {
    bool enabled = false;
    std::ofstream stream;
    std::chrono::steady_clock::time_point start;
    // Edge tracking
    std::array<bool, raylib::KEY_KP_EQUAL + 1> key_down{};
    std::array<bool, 8> mouse_down{};  // buttons 0-7
    float last_mouse_x = 0.0f;
    float last_mouse_y = 0.0f;
};

static RecorderState state;

long long elapsed_ms() {
    const auto now = std::chrono::steady_clock::now();
    const auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - state.start);
    return diff.count();
}

void write_line(long long timestamp_ms, const std::string& type,
                const std::string& code, const std::string& action) {
    if (!state.stream.is_open()) return;
    state.stream << timestamp_ms << "," << type << "," << code << "," << action
                 << "\n";
    state.stream.flush();
}
}  // namespace

void enable(const std::filesystem::path& path) {
    if (state.enabled) return;
    state.start = std::chrono::steady_clock::now();
    state.stream.open(path, std::ios::out | std::ios::trunc);
    if (!state.stream.is_open()) {
        log_error("Unable to open input recording file: {}", path.string());
        return;
    }
    state.key_down.fill(false);
    state.mouse_down.fill(false);
    state.last_mouse_x = 0.0f;
    state.last_mouse_y = 0.0f;
    state.enabled = true;
}

bool is_enabled() { return state.enabled; }

void record_state_change(const char* label, int value) {
    if (!state.enabled) return;
    const long long timestamp = elapsed_ms();
    write_line(timestamp, label, std::to_string(value), "state");
}

void shutdown() {
    if (!state.enabled) return;
    if (state.stream.is_open()) {
        state.stream.flush();
        state.stream.close();
    }
    state.enabled = false;
}

void update_poll() {
    if (!state.enabled) return;

    const long long timestamp = elapsed_ms();

    // Mouse move
    vec2 mouse_pos = ext::get_mouse_position();
    if (mouse_pos.x != state.last_mouse_x ||
        mouse_pos.y != state.last_mouse_y) {
        state.last_mouse_x = mouse_pos.x;
        state.last_mouse_y = mouse_pos.y;
        write_line(
            timestamp, "mouse_move",
            std::to_string(mouse_pos.x) + "_" + std::to_string(mouse_pos.y),
            "move");
        log_info("recorder: mouse move {} {}", mouse_pos.x, mouse_pos.y);
    }

    // Mouse buttons 0-7
    for (int b = 0; b < static_cast<int>(state.mouse_down.size()); ++b) {
        bool down =
            raylib::IsMouseButtonDown(static_cast<raylib::MouseButton>(b));
        if (down && !state.mouse_down[b]) {
            state.mouse_down[b] = true;
            write_line(timestamp, "mouse", std::to_string(b), "down");
            log_info("recorder: mouse {} down", b);
        } else if (!down && state.mouse_down[b]) {
            state.mouse_down[b] = false;
            write_line(timestamp, "mouse", std::to_string(b), "up");
            log_info("recorder: mouse {} up", b);
        }
    }

    // Keys 0..max
    for (int k = 0; k < static_cast<int>(state.key_down.size()); ++k) {
        bool down = raylib::IsKeyDown(k);
        if (down && !state.key_down[k]) {
            state.key_down[k] = true;
            write_line(timestamp, "key", std::to_string(k), "down");
            log_info("recorder: key {} down (poll)", k);
        } else if (!down && state.key_down[k]) {
            state.key_down[k] = false;
            write_line(timestamp, "key", std::to_string(k), "up");
            log_info("recorder: key {} up (poll)", k);
        }
    }
}

}  // namespace input_recorder
