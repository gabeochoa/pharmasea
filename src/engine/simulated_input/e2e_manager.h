#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "raylib.h"

#include "../../../vendor/afterhours/src/plugins/e2e_testing/input_injector.h"
#include "../../client_server_comm.h"
#include "../../network/network.h"
#include "../../save_game/save_game.h"
#include "../../system/core/system_manager.h"
#include "../../components/can_hold_item.h"
#include "../../components/can_order_drink.h"
#include "../../components/has_day_night_timer.h"
#include "../../components/is_bank.h"
#include "../../components/is_ai_controlled.h"
#include "../../components/is_customer.h"
#include "../../components/is_drink.h"
#include "../../components/is_floor_marker.h"
#include "../../components/is_store_spawned.h"
#include "../../components/is_trigger_area.h"
#include "../../entities/entity_query.h"
#include "../../globals.h"
#include "../app.h"
#include "../log.h"
#include "../runtime_globals.h"
#include "../settings.h"
#include "../statemanager.h"

namespace e2e_manager {

namespace {

struct ParsedCmd {
    std::string name;
    std::vector<std::string> args;
    std::string script;
    int line = 0;
    float wait_after = 0.f;
    float within_s = 30.f;
};

struct ScriptInfo {
    std::string path;
    std::string name;
    bool slow = false;
    bool xfail = false;
    size_t cmd_begin = 0;
    size_t cmd_end = 0;
    bool reported = false;
};

std::vector<ParsedCmd> commands;
std::vector<ScriptInfo> scripts;
size_t cmd_idx = 0;
float wait_remaining = 0.f;
float elapsed = 0.f;
float timeout_s = 1800.f;
bool active = false;
bool finished = false;
bool failed = false;
std::string fail_message;
int pending_idx = -1;
float pending_elapsed = 0.f;
int walk_idx = -1;
vec2 walk_target{0.f, 0.f};
float walk_elapsed = 0.f;

namespace ah = afterhours::testing::input_injector;
namespace fs = std::filesystem;

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

bool is_expect(const std::string& name) {
    return name.rfind("expect_", 0) == 0;
}

std::vector<ParsedCmd> parse_e2e_file(const std::string& path, bool& slow_out,
                                      bool& xfail_out) {
    std::vector<ParsedCmd> out;
    std::ifstream file(path);
    if (!file.is_open()) return out;

    std::string script_name = fs::path(path).filename().string();
    std::string line;
    int line_num = 0;
    constexpr float frame = 1.f / 60.f;
    slow_out = false;
    xfail_out = false;

    while (std::getline(file, line)) {
        line_num++;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::string trimmed = line;
        size_t start = trimmed.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        trimmed = trimmed.substr(start);
        if (trimmed.empty()) continue;
        if (trimmed[0] == '#') {
            if (trimmed.find("@slow") != std::string::npos) slow_out = true;
            if (trimmed.find("@xfail") != std::string::npos) xfail_out = true;
            continue;
        }

        ParsedCmd cmd;
        cmd.script = path;
        cmd.line = line_num;
        std::istringstream iss(trimmed);
        iss >> cmd.name;
        std::string arg;
        while (iss >> arg) cmd.args.push_back(arg);

        if (cmd.args.size() >= 2 &&
            cmd.args[cmd.args.size() - 2] == "within") {
            try {
                cmd.within_s = std::stof(cmd.args.back());
            } catch (...) {
            }
            cmd.args.pop_back();
            cmd.args.pop_back();
        }

        if (cmd.name == "wait") {
            float secs = 1.f;
            if (!cmd.args.empty()) {
                try {
                    secs = std::stof(cmd.args[0]);
                } catch (...) {
                }
            }
            if (secs <= 0) secs = 1.f;
            cmd.wait_after = secs;
        } else if (cmd.name == "wait_frames") {
            int frames = 1;
            if (!cmd.args.empty()) {
                try {
                    frames = std::stoi(cmd.args[0]);
                } catch (...) {
                }
            }
            if (frames <= 0) frames = 1;
            cmd.wait_after = frames * frame;
        } else if (cmd.name == "key") {
            cmd.wait_after = 3 * frame;
        } else if (is_expect(cmd.name)) {
            cmd.wait_after = 0.f;
        } else {
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

bool apply_op(const std::string& op, long actual, long expected) {
    if (op == "==" || op == "=") return actual == expected;
    if (op == "!=") return actual != expected;
    if (op == ">=") return actual >= expected;
    if (op == "<=") return actual <= expected;
    if (op == ">") return actual > expected;
    if (op == "<") return actual < expected;
    return false;
}

bool is_op(const std::string& s) {
    return s == "==" || s == "=" || s == "!=" || s == ">=" || s == "<=" ||
           s == ">" || s == "<";
}

bool parse_op_count(const std::vector<std::string>& args, size_t start,
                    std::string& op, long& count) {
    op = "==";
    size_t i = start;
    if (i < args.size() && is_op(args[i])) {
        op = args[i];
        i++;
    }
    if (i >= args.size()) return false;
    try {
        count = std::stol(args[i]);
    } catch (...) {
        return false;
    }
    return true;
}

std::string eval_expect(const ParsedCmd& cmd) {
    if (cmd.name == "expect_menu_state") {
        if (cmd.args.empty()) return "expect_menu_state requires a state name";
        std::string actual = std::string(MenuState::get().tostring());
        if (actual == cmd.args[0]) return "";
        return "expected menu state " + cmd.args[0] + " but was " + actual;
    }
    if (cmd.name == "expect_game_state") {
        if (cmd.args.empty()) return "expect_game_state requires a state name";
        std::string actual = std::string(GameState::get().tostring());
        if (actual == cmd.args[0]) return "";
        return "expected game state " + cmd.args[0] + " but was " + actual;
    }
    if (cmd.name == "expect_day") {
        std::string op;
        long expected = 0;
        if (!parse_op_count(cmd.args, 0, op, expected))
            return "expect_day requires [op] <n>";
        OptEntity e = EQ().whereHasComponent<HasDayNightTimer>().gen_first();
        if (!e.valid()) return "no HasDayNightTimer entity";
        long actual = e->get<HasDayNightTimer>().days_passed();
        if (apply_op(op, actual, expected)) return "";
        return "expected day " + op + " " + std::to_string(expected) +
               " but was " + std::to_string(actual);
    }
    if (cmd.name == "expect_phase") {
        if (cmd.args.empty()) return "expect_phase requires day|night";
        OptEntity e = EQ().whereHasComponent<HasDayNightTimer>().gen_first();
        if (!e.valid()) return "no HasDayNightTimer entity";
        const HasDayNightTimer& t = e->get<HasDayNightTimer>();
        std::string want = to_lower(cmd.args[0]);
        bool is_day_phase = t.is_bar_closed();
        if (want == "day" && is_day_phase) return "";
        if (want == "night" && !is_day_phase) return "";
        return std::string("expected phase ") + want + " but was " +
               (is_day_phase ? "day" : "night");
    }
    if (cmd.name == "expect_entity") {
        if (cmd.args.empty()) return "expect_entity requires a set name";
        std::string op;
        long expected = 0;
        if (!parse_op_count(cmd.args, 1, op, expected))
            return "expect_entity requires <Set> [op] <count>";
        std::string set_name = to_lower(cmd.args[0]);
        long actual = -1;
        if (set_name == "customer" || set_name == "customers") {
            actual = static_cast<long>(
                EQ().whereHasComponent<IsCustomer>().gen().size());
        } else if (set_name == "drink" || set_name == "drinks") {
            actual = static_cast<long>(
                EQ().whereHasComponent<IsDrink>().gen().size());
        } else if (set_name == "furniture") {
            actual = static_cast<long>(
                EQ().whereHasComponent<CanHoldItem>().gen().size());
        } else if (set_name == "player" || set_name == "players") {
            actual = static_cast<long>(
                EQ().whereType(EntityType::Player).gen().size());
        } else {
            return "unknown entity set '" + cmd.args[0] +
                   "' (want Customer|Drink|Furniture|Player)";
        }
        if (apply_op(op, actual, expected)) return "";
        return "expected " + cmd.args[0] + " count " + op + " " +
               std::to_string(expected) + " but was " +
               std::to_string(actual);
    }
    if (cmd.name == "expect_order") {
        if (cmd.args.size() < 2)
            return "expect_order requires <State> [op] <count>";
        std::string want = cmd.args[0];
        CanOrderDrink::OrderState state;
        if (want == "NeedsReset") {
            state = CanOrderDrink::OrderState::NeedsReset;
        } else if (want == "Ordering") {
            state = CanOrderDrink::OrderState::Ordering;
        } else if (want == "DrinkingNow") {
            state = CanOrderDrink::OrderState::DrinkingNow;
        } else if (want == "DoneDrinking") {
            state = CanOrderDrink::OrderState::DoneDrinking;
        } else {
            return "unknown order state '" + want + "'";
        }
        std::string op;
        long expected = 0;
        if (!parse_op_count(cmd.args, 1, op, expected))
            return "expect_order requires <State> [op] <count>";
        long actual = 0;
        RefEntities ents = EQ().whereHasComponent<CanOrderDrink>().gen();
        for (Entity& e : ents) {
            if (e.get<CanOrderDrink>().order_state == state) actual++;
        }
        if (apply_op(op, actual, expected)) return "";
        return "expected " + std::to_string(expected) + " customers in " +
               want + " (" + op + ") but was " + std::to_string(actual);
    }
    if (cmd.name == "expect_money") {
        std::string op;
        long expected = 0;
        if (!parse_op_count(cmd.args, 0, op, expected))
            return "expect_money requires [op] <amount>";
        OptEntity e = EQ().whereHasComponent<IsBank>().gen_first();
        if (!e.valid()) return "no IsBank entity";
        long actual = e->get<IsBank>().balance();
        if (apply_op(op, actual, expected)) return "";
        return "expected money " + op + " " + std::to_string(expected) +
               " but was " + std::to_string(actual);
    }
    if (cmd.name == "expect_served") {
        std::string op;
        long expected = 0;
        if (!parse_op_count(cmd.args, 0, op, expected))
            return "expect_served requires [op] <count>";
        long actual = 0;
        RefEntities ents = EQ(SystemManager::get().oldAll)
                               .whereHasComponent<CanOrderDrink>()
                               .gen();
        if (ents.empty()) {
            ents = EQ().whereHasComponent<CanOrderDrink>().gen();
        }
        for (Entity& e : ents) {
            if (e.get<CanOrderDrink>().num_drinks_drank() > 0) actual++;
        }
        static long served_high_water = 0;
        if (actual > served_high_water) served_high_water = actual;
        actual = served_high_water;
        if (apply_op(op, actual, expected)) return "";
        return "expected served customers " + op + " " +
               std::to_string(expected) + " but was " +
               std::to_string(actual);
    }
    if (cmd.name == "expect_furniture_near") {
        if (cmd.args.size() < 2)
            return "expect_furniture_near requires <x> <z>";
        float tx = 0.f;
        float tz = 0.f;
        try {
            tx = std::stof(cmd.args[0]);
            tz = std::stof(cmd.args[1]);
        } catch (...) {
            return "expect_furniture_near needs numeric coordinates";
        }
        RefEntities ents = EQ().whereHasComponent<CanHoldItem>().gen();
        for (Entity& e : ents) {
            vec2 pos = e.get<Transform>().as2();
            float dx = pos.x - tx;
            float dz = pos.y - tz;
            if (dx * dx + dz * dz <= 2.5f * 2.5f) return "";
        }
        return "no furniture within 2.5 of (" + cmd.args[0] + ", " +
               cmd.args[1] + ")";
    }
    if (cmd.name == "expect_withdrawals") {
        std::string op;
        long expected = 0;
        if (!parse_op_count(cmd.args, 0, op, expected))
            return "expect_withdrawals requires [op] <count>";
        OptEntity e = EQ().whereHasComponent<IsBank>().gen_first();
        if (!e.valid()) return "no IsBank entity";
        long actual = 0;
        const IsBank& bank = e->get<IsBank>();
        for (const IsBank::Transaction& t : bank.get_transactions()) {
            if (t.amount < 0) actual++;
        }
        if (apply_op(op, actual, expected)) return "";
        return "expected withdrawals " + op + " " +
               std::to_string(expected) + " but was " +
               std::to_string(actual);
    }
    if (cmd.name == "expect_cart") {
        std::string op;
        long expected = 0;
        if (!parse_op_count(cmd.args, 0, op, expected))
            return "expect_cart requires [op] <amount>";
        OptEntity e = EQ().whereHasComponent<IsBank>().gen_first();
        if (!e.valid()) return "no IsBank entity";
        long actual = e->get<IsBank>().cart();
        if (apply_op(op, actual, expected)) return "";
        return "expected cart " + op + " " + std::to_string(expected) +
               " but was " + std::to_string(actual);
    }
    if (cmd.name == "expect_save_slot") {
        if (cmd.args.empty()) return "expect_save_slot requires a slot number";
        int slot = 0;
        try {
            slot = std::stoi(cmd.args[0]);
        } catch (...) {
            return "expect_save_slot needs a number";
        }
        std::optional<save_game::SaveGameHeader> header =
            save_game::SaveGameManager::read_header(
                save_game::SaveGameManager::slot_path(slot));
        if (header.has_value()) return "";
        return "no valid save header in slot " + cmd.args[0] + " yet";
    }
    if (cmd.name == "expect_setting") {
        if (cmd.args.size() < 2)
            return "expect_setting requires <field> <value>";
        const std::string& field = cmd.args[0];
        const settings::Data& d = Settings::get().data;
        if (field == "username" || field == "lang_name") {
            std::string actual =
                field == "username" ? d.username : d.lang_name;
            if (actual == cmd.args[1]) return "";
            return "expected setting " + field + " == " + cmd.args[1] +
                   " but was " + actual;
        }
        if (field == "master_volume" || field == "music_volume" ||
            field == "sound_volume") {
            float actual = field == "master_volume" ? d.master_volume
                           : field == "music_volume" ? d.music_volume
                                                     : d.sound_volume;
            float expected = 0.f;
            try {
                expected = std::stof(cmd.args[1]);
            } catch (...) {
                return "expect_setting " + field + " needs a number";
            }
            if (std::fabs(actual - expected) < 0.001f) return "";
            return "expected setting " + field + " == " + cmd.args[1] +
                   " but was " + std::to_string(actual);
        }
        if (field == "vsync_enabled" || field == "enable_postprocessing" ||
            field == "enable_lighting" || field == "isFullscreen") {
            bool actual = field == "vsync_enabled" ? d.vsync_enabled
                          : field == "enable_postprocessing"
                              ? d.enable_postprocessing
                              : field == "enable_lighting" ? d.enable_lighting
                                                           : d.isFullscreen;
            bool expected = cmd.args[1] == "true" || cmd.args[1] == "1";
            if (actual == expected) return "";
            return "expected setting " + field + " == " + cmd.args[1] +
                   " but was " + (actual ? "true" : "false");
        }
        return "unknown setting field '" + field + "'";
    }
    return "unknown expect command '" + cmd.name + "'";
}

bool capture_png_to(const fs::path& out) {
    raylib::Image image =
        raylib::LoadImageFromTexture(App::get().get_main_texture());
    if (image.data == nullptr) return false;
    raylib::ImageFlipVertical(&image);
    int file_size = 0;
    unsigned char* png_data =
        raylib::ExportImageToMemory(image, ".png", &file_size);
    raylib::UnloadImage(image);
    if (png_data == nullptr || file_size <= 0) return false;
    std::ofstream file(out, std::ios::binary);
    if (!file.is_open()) {
        raylib::MemFree(png_data);
        return false;
    }
    file.write(reinterpret_cast<const char*>(png_data), file_size);
    raylib::MemFree(png_data);
    return file.good();
}

void capture_failure_screenshot(const ParsedCmd& cmd) {
    fs::path script_path(cmd.script);
    fs::path dir = script_path.has_parent_path()
                       ? script_path.parent_path()
                       : fs::path("tests/e2e");
    std::string stem = script_path.stem().string();
    fs::path out = dir / (stem + "_FAIL.png");
    if (capture_png_to(out)) {
        log_info("e2e: failure screenshot -> {}", out.string());
    } else {
        log_warn("e2e: failure screenshot failed for {}", out.string());
    }
}

void write_results_json(bool passed) {
    if (E2E_RESULTS_JSON.empty()) return;
    std::ofstream out(E2E_RESULTS_JSON);
    if (!out.is_open()) return;
    out << "{\"passed\":" << (passed ? "true" : "false")
        << ",\"elapsed_s\":" << elapsed << ",\"scripts\":[";
    for (size_t i = 0; i < scripts.size(); i++) {
        if (i) out << ",";
        out << "{\"name\":\"" << scripts[i].name << "\",\"slow\":"
            << (scripts[i].slow ? "true" : "false") << "}";
    }
    out << "],\"failure\":\"";
    for (char c : fail_message) {
        if (c == '"' || c == '\\') out << '\\';
        out << c;
    }
    out << "\"}\n";
}

void finish(bool passed, const std::string& message) {
    if (finished) return;
    finished = true;
    active = false;
    failed = !passed;
    fail_message = message;
    E2E_EXIT_CODE = passed ? 0 : 1;
    if (passed) {
        for (ScriptInfo& s : scripts) {
            if (!s.reported) {
                fmt::print(stderr, "[PASS] {}\n", s.name);
                s.reported = true;
            }
        }
        log_info("E2E: PASSED -- all {} commands completed in {:.1f}s",
                 commands.size(), elapsed);
        fmt::print(stderr, "E2E: PASSED in {:.1f}s\n", elapsed);
    }
    write_results_json(passed);
    App::get().close();
}

void fail_at(const ParsedCmd& cmd, const std::string& reason) {
    std::string where = cmd.script + ":" + std::to_string(cmd.line);
    log_warn("e2e: FAIL {}: {} '{}': {}", where, cmd.name,
             cmd.args.empty() ? "" : cmd.args[0], reason);
    fmt::print(stderr, "[FAIL] {}:{} {}: {}\n",
               fs::path(cmd.script).filename().string(), cmd.line, cmd.name,
               reason);
    capture_failure_screenshot(cmd);
    for (ScriptInfo& s : scripts) {
        if (!s.reported && cmd_idx >= s.cmd_begin && cmd_idx < s.cmd_end) {
            s.reported = true;
        }
    }
    finish(false, where + " " + cmd.name + ": " + reason);
}

bool dispatch_action(const ParsedCmd& cmd) {
    if (cmd.name == "wait" || cmd.name == "wait_frames") return true;
    if (cmd.name == "key") {
        if (cmd.args.empty()) {
            fail_at(cmd, "key requires a key name");
            return false;
        }
        int code = key_name_to_code(cmd.args[0]);
        if (code < 0) {
            fail_at(cmd, "unknown key '" + cmd.args[0] + "'");
            return false;
        }
        ah::set_key_down(code);
        log_info("e2e: key {} (code {})", cmd.args[0], code);
        return true;
    }
    if (cmd.name == "hold_key" || cmd.name == "release_key") {
        if (cmd.args.empty()) {
            fail_at(cmd, cmd.name + " requires a key name");
            return false;
        }
        int code = key_name_to_code(cmd.args[0]);
        if (code < 0) {
            fail_at(cmd, "unknown key '" + cmd.args[0] + "'");
            return false;
        }
        if (cmd.name == "hold_key") {
            ah::set_key_down(code);
        } else {
            ah::set_key_up(code);
        }
        log_info("e2e: {} {} (code {})", cmd.name, cmd.args[0], code);
        return true;
    }
    if (cmd.name == "whereami") {
        Map* map = globals::world_map();
        if (map && !map->local_players_NOT_SERIALIZED.empty()) {
            Entity& player = *map->local_players_NOT_SERIALIZED[0];
            vec2 pos = player.get<Transform>().as2();
            std::string facing = std::string(magic_enum::enum_name(
                player.get<Transform>().face_direction()));
            log_info("e2e: whereami player at ({:.2f}, {:.2f}) facing {}",
                     pos.x, pos.y, facing);
            fmt::print(stderr,
                       "whereami: player at ({:.2f}, {:.2f}) facing {}\n",
                       pos.x, pos.y, facing);
        } else {
            log_warn("e2e: whereami found no local player");
        }
        return true;
    }
    if (cmd.name == "holding") {
        Map* map = globals::world_map();
        Entity* found = nullptr;
        if (map) {
            for (std::shared_ptr<Entity>& p :
                 map->local_players_NOT_SERIALIZED) {
                if (p && !p->is_missing<CanHoldItem>() &&
                    p->get<CanHoldItem>().item().valid()) {
                    found = p.get();
                    break;
                }
            }
            if (!found) {
                for (std::shared_ptr<Entity>& p :
                     map->remote_players_NOT_SERIALIZED) {
                    if (p && !p->is_missing<CanHoldItem>() &&
                        p->get<CanHoldItem>().item().valid()) {
                        found = p.get();
                        break;
                    }
                }
            }
        }
        if (!found) {
            RefEntities players = EQ().whereType(EntityType::Player).gen();
            for (Entity& p : players) {
                if (!p.is_missing<CanHoldItem>() &&
                    p.get<CanHoldItem>().item().valid()) {
                    found = &p;
                    break;
                }
            }
        }
        if (!found) {
            fmt::print(stderr, "holding: <empty>\n");
            return true;
        }
        {
            Entity& item = found->get<CanHoldItem>().item().asE();
            std::string label = "Unknown";
            for (size_t i = 0; i < magic_enum::enum_count<EntityType>(); i++) {
                EntityType t = magic_enum::enum_value<EntityType>(i);
                if (item.hasTag(t)) {
                    label = std::string(magic_enum::enum_name(t));
                    break;
                }
            }
            std::string drink = "none";
            if (item.has<IsDrink>() &&
                item.get<IsDrink>().underlying.has_value()) {
                drink = std::string(magic_enum::enum_name(
                    item.get<IsDrink>().underlying.value()));
            }
            fmt::print(stderr, "holding: {} drink={}\n", label, drink);
        }
        return true;
    }
    if (cmd.name == "dump_areas") {
        RefEntities markers = EQ()
                                  .include_store_entities()
                                  .whereHasComponent<IsFloorMarker>()
                                  .gen();
        for (Entity& e : markers) {
            vec2 pos = e.get<Transform>().as2();
            fmt::print(stderr, "marker: {} at ({:.1f}, {:.1f})\n",
                       magic_enum::enum_name(e.get<IsFloorMarker>().type),
                       pos.x, pos.y);
        }
        RefEntities triggers = EQ()
                                   .include_store_entities()
                                   .whereHasComponent<IsTriggerArea>()
                                   .gen();
        for (Entity& e : triggers) {
            vec2 pos = e.get<Transform>().as2();
            fmt::print(stderr, "trigger: {} at ({:.1f}, {:.1f})\n",
                       magic_enum::enum_name(e.get<IsTriggerArea>().type),
                       pos.x, pos.y);
        }
        return true;
    }
    if (cmd.name == "whereami_server") {
        RefEntities players =
            EQ(SystemManager::get().oldAll).whereType(EntityType::Player).gen();
        for (Entity& p : players) {
            vec2 pos = p.get<Transform>().as2();
            std::string facing = std::string(
                magic_enum::enum_name(p.get<Transform>().face_direction()));
            fmt::print(stderr,
                       "whereami_server: player at ({:.2f}, {:.2f}) facing "
                       "{}\n",
                       pos.x, pos.y, facing);
        }
        if (players.empty()) {
            fmt::print(stderr, "whereami_server: no server players\n");
        }
        return true;
    }
    if (cmd.name == "orders") {
        RefEntities ents = EQ().whereHasComponent<CanOrderDrink>().gen();
        for (Entity& e : ents) {
            const CanOrderDrink& cod = e.get<CanOrderDrink>();
            std::string ai = "?";
            if (e.has<IsAIControlled>()) {
                ai = std::string(magic_enum::enum_name(
                    e.get<IsAIControlled>().state));
            }
            fmt::print(stderr,
                       "order: drink={} state={} ai={} drank={}\n",
                       magic_enum::enum_name(cod.order()),
                       magic_enum::enum_name(cod.order_state), ai,
                       cod.num_drinks_drank());
        }
        return true;
    }
    if (cmd.name == "dump_store") {
        RefEntities ents = EQ(EntityHelper::get_server_collection())
                               .whereHasComponent<IsStoreSpawned>()
                               .gen();
        for (Entity& e : ents) {
            vec2 pos = e.get<Transform>().as2();
            std::string label = "Unknown";
            int price = -1;
            for (size_t i = 0; i < magic_enum::enum_count<EntityType>(); i++) {
                EntityType t = magic_enum::enum_value<EntityType>(i);
                if (e.hasTag(t)) {
                    label = std::string(magic_enum::enum_name(t));
                    price = get_price_for_entity_type(t);
                    break;
                }
            }
            fmt::print(stderr, "store: {} price={} at ({:.1f}, {:.1f})\n",
                       label, price, pos.x, pos.y);
        }
        return true;
    }
    if (cmd.name == "dump_walls") {
        RefEntities ents = EQ().whereType(EntityType::Wall).gen();
        for (Entity& e : ents) {
            vec2 pos = e.get<Transform>().as2();
            fmt::print(stderr, "wall: ({:.1f}, {:.1f})\n", pos.x, pos.y);
        }
        return true;
    }
    if (cmd.name == "dump_entities") {
        RefEntities ents = EQ().whereHasComponent<CanHoldItem>().gen();
        for (Entity& e : ents) {
            vec2 pos = e.get<Transform>().as2();
            std::string label = "Unknown";
            for (size_t i = 0; i < magic_enum::enum_count<EntityType>(); i++) {
                EntityType t = magic_enum::enum_value<EntityType>(i);
                if (e.hasTag(t)) {
                    label = std::string(magic_enum::enum_name(t));
                    break;
                }
            }
            std::string held = "empty";
            if (!e.is_missing<CanHoldItem>()) {
                OptEntity item_opt = e.get<CanHoldItem>().item();
                if (item_opt.valid()) {
                    Entity& item = item_opt.asE();
                    held = "Unknown";
                    for (size_t j = 0;
                         j < magic_enum::enum_count<EntityType>(); j++) {
                        EntityType t = magic_enum::enum_value<EntityType>(j);
                        if (item.hasTag(t)) {
                            held = std::string(magic_enum::enum_name(t));
                            break;
                        }
                    }
                    if (item.has<IsDrink>() &&
                        item.get<IsDrink>().underlying.has_value()) {
                        held += "(" +
                                std::string(magic_enum::enum_name(
                                    item.get<IsDrink>().underlying.value())) +
                                ")";
                    }
                }
            }
            fmt::print(stderr, "dump: {} at ({:.1f}, {:.1f}) holding {}\n",
                       label, pos.x, pos.y, held);
        }
        return true;
    }
    if (cmd.name == "screenshot") {
        std::string name = cmd.args.empty() ? "e2e_shot" : cmd.args[0];
        fs::path dir = fs::path(cmd.script).has_parent_path()
                           ? fs::path(cmd.script).parent_path()
                           : fs::path("tests/e2e");
        fs::path out = dir / (name + ".png");
        if (!capture_png_to(out)) {
            fail_at(cmd, "screenshot capture failed");
            return false;
        }
        log_info("e2e: screenshot -> {}", out.string());
        return true;
    }
    if (cmd.name == "set_setting") {
        if (cmd.args.size() < 2) {
            fail_at(cmd, "set_setting requires <field> <value>");
            return false;
        }
        const std::string& field = cmd.args[0];
        settings::Data& d = Settings::get().data;
        try {
            if (field == "username") {
                d.username = cmd.args[1];
            } else if (field == "lang_name") {
                d.lang_name = cmd.args[1];
            } else if (field == "master_volume") {
                d.master_volume = std::stof(cmd.args[1]);
            } else if (field == "music_volume") {
                d.music_volume = std::stof(cmd.args[1]);
            } else if (field == "sound_volume") {
                d.sound_volume = std::stof(cmd.args[1]);
            } else if (field == "vsync_enabled") {
                d.vsync_enabled = cmd.args[1] == "true" || cmd.args[1] == "1";
            } else if (field == "enable_postprocessing") {
                d.enable_postprocessing =
                    cmd.args[1] == "true" || cmd.args[1] == "1";
            } else if (field == "enable_lighting") {
                d.enable_lighting = cmd.args[1] == "true" || cmd.args[1] == "1";
            } else {
                fail_at(cmd, "unknown setting field '" + field + "'");
                return false;
            }
        } catch (...) {
            fail_at(cmd, "bad value for setting '" + field + "'");
            return false;
        }
        return true;
    }
    if (cmd.name == "save_settings") {
        Settings::get().write_save_file();
        return true;
    }
    if (cmd.name == "reload_settings") {
        Settings::get().load_save_file();
        return true;
    }
    if (cmd.name == "save_slot") {
        if (cmd.args.empty()) {
            fail_at(cmd, "save_slot requires a slot number");
            return false;
        }
        int slot = 0;
        try {
            slot = std::stoi(cmd.args[0]);
        } catch (...) {
            fail_at(cmd, "save_slot needs a number");
            return false;
        }
        if (!network_info || !network_info->client) {
            fail_at(cmd, "save_slot requires an active network client");
            return false;
        }
        network_info->client->send_save_slot_request(slot);
        log_info("e2e: queued save_slot {} request", slot);
        return true;
    }
    fail_at(cmd, "unknown command '" + cmd.name + "'");
    return false;
}

}  // namespace

inline void init() {
    if (!E2E_ENABLED) return;

    std::vector<std::string> paths;
    if (!E2E_DIR.empty()) {
        if (fs::is_directory(E2E_DIR)) {
            for (const auto& entry : fs::directory_iterator(E2E_DIR)) {
                if (entry.path().extension() == ".e2e")
                    paths.push_back(entry.path().string());
            }
        }
        std::sort(paths.begin(), paths.end());
    } else if (!E2E_SCRIPT.empty()) {
        paths.push_back(E2E_SCRIPT);
    }

    for (const std::string& p : paths) {
        bool slow = false;
        bool xfail = false;
        std::vector<ParsedCmd> cmds = parse_e2e_file(p, slow, xfail);
        if (xfail && !E2E_DIR.empty()) {
            log_info("E2E: skipping @xfail script {} in dir mode", p);
            continue;
        }
        if (cmds.empty()) {
            log_warn("E2E: script {} has no commands", p);
            continue;
        }
        ScriptInfo info;
        info.path = p;
        info.name = fs::path(p).filename().string();
        info.slow = slow;
        info.xfail = xfail;
        info.cmd_begin = commands.size();
        for (ParsedCmd& c : cmds) commands.push_back(std::move(c));
        info.cmd_end = commands.size();
        scripts.push_back(info);
    }

    if (commands.empty()) {
        log_warn("E2E: no commands loaded, disabling");
        E2E_ENABLED = false;
        E2E_EXIT_CODE = 1;
        return;
    }

    active = true;
    log_info("E2E: loaded {} commands from {} script(s)", commands.size(),
             scripts.size());
}

inline void update(float dt) {
    if (!active || finished) return;

    elapsed += dt;
    if (elapsed > timeout_s) {
        if (cmd_idx < commands.size()) {
            fail_at(commands[cmd_idx], "global timeout after " +
                                           std::to_string(timeout_s) + "s");
        } else {
            finish(false, "global timeout");
        }
        return;
    }

    if (walk_idx >= 0) {
        const ParsedCmd& cmd = commands[static_cast<size_t>(walk_idx)];
        Map* map = globals::world_map();
        if (!map || map->local_players_NOT_SERIALIZED.empty()) {
            fail_at(cmd, "walk_to found no local player");
            return;
        }
        Entity& player = *map->local_players_NOT_SERIALIZED[0];
        vec2 pos = player.get<Transform>().as2();
        float dx = walk_target.x - pos.x;
        float dz = walk_target.y - pos.y;
        float dist = std::sqrt(dx * dx + dz * dz);
        auto set_move = [](const char* name, bool down) {
            int code = key_name_to_code(name);
            if (code < 0) return;
            if (down) {
                ah::set_key_down(code);
            } else {
                ah::set_key_up(code);
            }
        };
        if (dist < 0.45f) {
            set_move("W", false);
            set_move("A", false);
            set_move("S", false);
            set_move("D", false);
            log_info("e2e: walk_to arrived at ({:.1f}, {:.1f})", pos.x,
                     pos.y);
            walk_idx = -1;
            walk_elapsed = 0.f;
            cmd_idx++;
            return;
        }
        walk_elapsed += dt;
        if (walk_elapsed >= cmd.within_s) {
            set_move("W", false);
            set_move("A", false);
            set_move("S", false);
            set_move("D", false);
            fail_at(cmd, "walk_to timed out at (" + std::to_string(pos.x) +
                             ", " + std::to_string(pos.y) + ")");
            return;
        }
        set_move("D", dx > 0.3f);
        set_move("A", dx < -0.3f);
        set_move("S", dz > 0.3f);
        set_move("W", dz < -0.3f);
        return;
    }

    if (pending_idx >= 0) {
        const ParsedCmd& cmd = commands[static_cast<size_t>(pending_idx)];
        std::string mismatch = eval_expect(cmd);
        if (mismatch.empty()) {
            log_info("e2e: {} passed", cmd.name);
            pending_idx = -1;
            pending_elapsed = 0.f;
            cmd_idx++;
        } else {
            pending_elapsed += dt;
            if (pending_elapsed >= cmd.within_s) {
                fail_at(cmd, mismatch);
            }
        }
        return;
    }

    if (wait_remaining > 0.f) {
        wait_remaining -= dt;
        return;
    }

    if (cmd_idx >= commands.size()) {
        finish(true, "");
        return;
    }

    for (ScriptInfo& s : scripts) {
        if (!s.reported && cmd_idx >= s.cmd_end && s.cmd_end > s.cmd_begin) {
            fmt::print(stderr, "[PASS] {}\n", s.name);
            log_info("[PASS] {}", s.name);
            s.reported = true;
        }
    }

    const ParsedCmd& cmd = commands[cmd_idx];
    log_info("e2e: [{}/{}] {} (line {})", cmd_idx + 1, commands.size(),
             cmd.name, cmd.line);

    if (cmd.name == "walk_to") {
        if (cmd.args.size() < 2) {
            fail_at(cmd, "walk_to requires <x> <z>");
            return;
        }
        try {
            walk_target = vec2{std::stof(cmd.args[0]), std::stof(cmd.args[1])};
        } catch (...) {
            fail_at(cmd, "walk_to needs numeric coordinates");
            return;
        }
        walk_idx = static_cast<int>(cmd_idx);
        walk_elapsed = 0.f;
        return;
    }

    if (is_expect(cmd.name)) {
        std::string mismatch = eval_expect(cmd);
        if (mismatch.empty()) {
            cmd_idx++;
        } else if (cmd.within_s <= 0.f) {
            fail_at(cmd, mismatch);
        } else {
            pending_idx = static_cast<int>(cmd_idx);
            pending_elapsed = 0.f;
        }
        return;
    }

    if (!dispatch_action(cmd)) return;
    wait_remaining = cmd.wait_after;
    cmd_idx++;
}

}  // namespace e2e_manager
