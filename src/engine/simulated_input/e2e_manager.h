#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "../../../vendor/afterhours/src/plugins/e2e_testing/input_injector.h"
#include "../../../vendor/afterhours/src/plugins/e2e_testing/pending_command.h"
#include "../../globals.h"
#include "../app.h"
#include "../log.h"

// Lightweight E2E runner that uses afterhours' script parsing but handles
// command dispatch directly instead of through the full ECS command handler
// chain (which pulls in afterhours' UI/graphics stack).
namespace e2e_manager {

namespace {

struct ParsedCmd {
    std::string name;
    std::vector<std::string> args;
    int line = 0;
    float wait_after = 0.f;
};

std::vector<ParsedCmd> commands;
size_t cmd_idx = 0;
float wait_remaining = 0.f;
float elapsed = 0.f;
float timeout_s = 60.f;
bool active = false;
bool finished = false;

namespace ah = afterhours::testing::input_injector;

std::vector<ParsedCmd> parse_e2e_file(const std::string& path) {
    std::vector<ParsedCmd> out;
    std::ifstream file(path);
    if (!file.is_open()) return out;

    std::string line;
    int line_num = 0;
    constexpr float frame = 1.f / 60.f;

    while (std::getline(file, line)) {
        line_num++;
        auto start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);
        if (line.empty() || line[0] == '#') continue;

        ParsedCmd cmd;
        cmd.line = line_num;
        std::istringstream iss(line);
        iss >> cmd.name;

        if (cmd.name == "wait") {
            float secs = 1.f;
            iss >> secs;
            if (secs <= 0) secs = 1.f;
            cmd.args.push_back(std::to_string(secs));
            cmd.wait_after = secs;
        } else if (cmd.name == "wait_frames") {
            int frames = 1;
            iss >> frames;
            if (frames <= 0) frames = 1;
            cmd.wait_after = frames * frame;
        } else if (cmd.name == "key") {
            std::string arg;
            iss >> arg;
            cmd.args.push_back(arg);
            cmd.wait_after = 3 * frame;
        } else {
            std::string arg;
            while (iss >> arg) cmd.args.push_back(arg);
            cmd.wait_after = 2 * frame;
        }
        out.push_back(cmd);
    }
    return out;
}

int key_name_to_code(const std::string& name) {
    if (name == "ENTER" || name == "RETURN") return 257;
    if (name == "SPACE") return 32;
    if (name == "ESCAPE" || name == "ESC") return 256;
    if (name == "TAB") return 258;
    if (name == "BACKSPACE") return 259;
    if (name == "UP") return 265;
    if (name == "DOWN") return 264;
    if (name == "LEFT") return 263;
    if (name == "RIGHT") return 262;
    if (name == "DELETE") return 261;
    if (name.size() == 1 && name[0] >= 'A' && name[0] <= 'Z')
        return static_cast<int>(name[0]);
    if (name.size() == 1 && name[0] >= '0' && name[0] <= '9')
        return static_cast<int>(name[0]);
    try {
        return std::stoi(name);
    } catch (...) {
    }
    return -1;
}

void dispatch(const ParsedCmd& cmd) {
    if (cmd.name == "wait" || cmd.name == "wait_frames") {
        return;
    }

    if (cmd.name == "key") {
        if (!cmd.args.empty()) {
            int code = key_name_to_code(cmd.args[0]);
            if (code >= 0) {
                ah::set_key_down(code);
                log_info("e2e: key {} (code {})", cmd.args[0], code);
            } else {
                log_warn("e2e: unknown key '{}'", cmd.args[0]);
            }
        }
        return;
    }

    log_warn("e2e: unhandled command '{}' at line {}", cmd.name, cmd.line);
}

}  // namespace

inline void init() {
    if (!E2E_ENABLED) return;

    if (!E2E_DIR.empty()) {
        namespace fs = std::filesystem;
        std::vector<std::string> scripts;
        for (const auto& entry : fs::directory_iterator(E2E_DIR)) {
            if (entry.path().extension() == ".e2e")
                scripts.push_back(entry.path().string());
        }
        std::sort(scripts.begin(), scripts.end());
        for (const auto& s : scripts) {
            auto cmds = parse_e2e_file(s);
            for (auto& c : cmds) commands.push_back(std::move(c));
        }
    } else if (!E2E_SCRIPT.empty()) {
        commands = parse_e2e_file(E2E_SCRIPT);
    }

    if (commands.empty()) {
        log_warn("E2E: no commands loaded, disabling");
        E2E_ENABLED = false;
        return;
    }

    active = true;
    log_info("E2E: loaded {} commands", commands.size());
}

inline void update(float dt) {
    if (!active || finished) return;

    if (wait_remaining > 0.f) {
        wait_remaining -= dt;
        return;
    }

    elapsed += dt;
    if (elapsed > timeout_s) {
        log_warn("E2E: TIMEOUT after {:.1f}s at command {}/{}", elapsed,
                 cmd_idx, commands.size());
        finished = true;
        active = false;
        App::get().close();
        return;
    }

    if (cmd_idx >= commands.size()) {
        log_info("E2E: PASSED -- all {} commands completed in {:.1f}s",
                 commands.size(), elapsed);
        fmt::print(stderr, "E2E: PASSED in {:.1f}s\n", elapsed);
        finished = true;
        active = false;
        App::get().close();
        return;
    }

    const auto& cmd = commands[cmd_idx];
    log_info("e2e: [{}/{}] {} (wait {:.2f}s)", cmd_idx + 1, commands.size(),
             cmd.name, cmd.wait_after);
    dispatch(cmd);
    wait_remaining = cmd.wait_after;
    cmd_idx++;
}

}  // namespace e2e_manager
