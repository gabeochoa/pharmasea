#pragma once

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "raylib.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#include "../../../vendor/afterhours/src/plugins/e2e_testing/e2e_testing.h"
#pragma GCC diagnostic pop
#include "../../client_server_comm.h"
#include "../../components/can_hold_item.h"
#include "../../components/can_order_drink.h"
#include "../../components/has_day_night_timer.h"
#include "../../components/is_ai_controlled.h"
#include "../../components/is_bank.h"
#include "../../components/is_customer.h"
#include "../../components/is_drink.h"
#include "../../components/is_floor_marker.h"
#include "../../components/is_store_spawned.h"
#include "../../components/is_trigger_area.h"
#include "../../entities/entity_query.h"
#include "../../globals.h"
#include "../../network/network.h"
#include "../../save_game/save_game.h"
#include "../app.h"
#include "../log.h"
#include "../runtime_globals.h"
#include "../settings.h"
#include "../statemanager.h"
#include "../../system/core/system_manager.h"

namespace e2e_manager {

namespace {

namespace testing = afterhours::testing;
namespace fs = std::filesystem;

testing::E2ERunner runner;
afterhours::SystemManager pack;
bool pack_ready = false;
bool active = false;
bool finish_handled = false;
std::string current_script;

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

bool is_op(const std::string& s) {
    return s == "==" || s == "=" || s == "!=" || s == ">=" || s == "<=" ||
           s == ">" || s == "<";
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

struct EvalResult {
    bool pass = false;
    bool malformed = false;
    std::string message;
};

EvalResult mismatch(std::string msg) {
    return EvalResult{.pass = false, .malformed = false, .message = msg};
}
EvalResult malformed(std::string msg) {
    return EvalResult{.pass = false, .malformed = true, .message = msg};
}
EvalResult passed() { return EvalResult{.pass = true}; }

EvalResult eval_expect(testing::PendingE2ECommand& cmd) {
    if (cmd.name == "expect_menu_state") {
        if (cmd.args.empty())
            return malformed("expect_menu_state requires a state name");
        std::string actual = std::string(MenuState::get().tostring());
        if (actual == cmd.args[0]) return passed();
        return mismatch("expected menu state " + cmd.args[0] + " but was " +
                        actual);
    }
    if (cmd.name == "expect_game_state") {
        if (cmd.args.empty())
            return malformed("expect_game_state requires a state name");
        std::string actual = std::string(GameState::get().tostring());
        if (actual == cmd.args[0]) return passed();
        return mismatch("expected game state " + cmd.args[0] + " but was " +
                        actual);
    }
    if (cmd.name == "expect_day") {
        std::string op;
        long expected = 0;
        if (!parse_op_count(cmd.args, 0, op, expected))
            return malformed("expect_day requires [op] <n>");
        OptEntity e = EQ().whereHasComponent<HasDayNightTimer>().gen_first();
        if (!e.valid()) return mismatch("no HasDayNightTimer entity");
        long actual = e->get<HasDayNightTimer>().days_passed();
        if (apply_op(op, actual, expected)) return passed();
        return mismatch("expected day " + op + " " +
                        std::to_string(expected) + " but was " +
                        std::to_string(actual));
    }
    if (cmd.name == "expect_phase") {
        if (cmd.args.empty()) return malformed("expect_phase requires day|night");
        OptEntity e = EQ().whereHasComponent<HasDayNightTimer>().gen_first();
        if (!e.valid()) return mismatch("no HasDayNightTimer entity");
        const HasDayNightTimer& t = e->get<HasDayNightTimer>();
        std::string want = to_lower(cmd.args[0]);
        if (want != "day" && want != "night")
            return malformed("expect_phase requires day|night");
        bool is_day_phase = t.is_bar_closed();
        if (want == "day" && is_day_phase) return passed();
        if (want == "night" && !is_day_phase) return passed();
        return mismatch(std::string("expected phase ") + want + " but was " +
                        (is_day_phase ? "day" : "night"));
    }
    if (cmd.name == "expect_entity") {
        if (cmd.args.empty()) return malformed("expect_entity requires a set name");
        std::string op;
        long expected = 0;
        if (!parse_op_count(cmd.args, 1, op, expected))
            return malformed("expect_entity requires <Set> [op] <count>");
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
            return malformed("unknown entity set '" + cmd.args[0] +
                             "' (want Customer|Drink|Furniture|Player)");
        }
        if (apply_op(op, actual, expected)) return passed();
        return mismatch("expected " + cmd.args[0] + " count " + op + " " +
                        std::to_string(expected) + " but was " +
                        std::to_string(actual));
    }
    if (cmd.name == "expect_order") {
        if (cmd.args.size() < 2)
            return malformed("expect_order requires <State> [op] <count>");
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
            return malformed("unknown order state '" + want + "'");
        }
        std::string op;
        long expected = 0;
        if (!parse_op_count(cmd.args, 1, op, expected))
            return malformed("expect_order requires <State> [op] <count>");
        long actual = 0;
        RefEntities ents = EQ().whereHasComponent<CanOrderDrink>().gen();
        for (Entity& e : ents) {
            if (e.get<CanOrderDrink>().order_state == state) actual++;
        }
        if (apply_op(op, actual, expected)) return passed();
        return mismatch("expected " + std::to_string(expected) +
                        " customers in " + want + " (" + op + ") but was " +
                        std::to_string(actual));
    }
    if (cmd.name == "expect_served") {
        std::string op;
        long expected = 0;
        if (!parse_op_count(cmd.args, 0, op, expected))
            return malformed("expect_served requires [op] <count>");
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
        if (apply_op(op, actual, expected)) return passed();
        return mismatch("expected served customers " + op + " " +
                        std::to_string(expected) + " but was " +
                        std::to_string(actual));
    }
    if (cmd.name == "expect_money" || cmd.name == "expect_cart" ||
        cmd.name == "expect_withdrawals") {
        std::string op;
        long expected = 0;
        if (!parse_op_count(cmd.args, 0, op, expected))
            return malformed(cmd.name + " requires [op] <amount>");
        OptEntity e = EQ().whereHasComponent<IsBank>().gen_first();
        if (!e.valid()) return mismatch("no IsBank entity");
        const IsBank& bank = e->get<IsBank>();
        long actual = 0;
        if (cmd.name == "expect_money") {
            actual = bank.balance();
        } else if (cmd.name == "expect_cart") {
            actual = bank.cart();
        } else {
            for (const IsBank::Transaction& t : bank.get_transactions()) {
                if (t.amount < 0) actual++;
            }
        }
        if (apply_op(op, actual, expected)) return passed();
        return mismatch("expected " + cmd.name + " " + op + " " +
                        std::to_string(expected) + " but was " +
                        std::to_string(actual));
    }
    if (cmd.name == "expect_save_slot") {
        if (cmd.args.empty())
            return malformed("expect_save_slot requires a slot number");
        int slot = 0;
        try {
            slot = std::stoi(cmd.args[0]);
        } catch (...) {
            return malformed("expect_save_slot needs a number");
        }
        std::optional<save_game::SaveGameHeader> header =
            save_game::SaveGameManager::read_header(
                save_game::SaveGameManager::slot_path(slot));
        if (header.has_value()) return passed();
        return mismatch("no valid save header in slot " + cmd.args[0] +
                        " yet");
    }
    if (cmd.name == "expect_furniture_near") {
        if (cmd.args.size() < 2)
            return malformed("expect_furniture_near requires <x> <z>");
        float tx = 0.f;
        float tz = 0.f;
        try {
            tx = std::stof(cmd.args[0]);
            tz = std::stof(cmd.args[1]);
        } catch (...) {
            return malformed("expect_furniture_near needs numbers");
        }
        RefEntities ents = EQ().whereHasComponent<CanHoldItem>().gen();
        for (Entity& e : ents) {
            vec2 pos = e.get<Transform>().as2();
            float dx = pos.x - tx;
            float dz = pos.y - tz;
            if (dx * dx + dz * dz <= 2.5f * 2.5f) return passed();
        }
        return mismatch("no furniture within 2.5 of (" + cmd.args[0] +
                        ", " + cmd.args[1] + ")");
    }
    if (cmd.name == "expect_setting") {
        if (cmd.args.size() < 2)
            return malformed("expect_setting requires <field> <value>");
        const std::string& field = cmd.args[0];
        const settings::Data& d = Settings::get().data;
        if (field == "username" || field == "lang_name") {
            std::string actual =
                field == "username" ? d.username : d.lang_name;
            if (actual == cmd.args[1]) return passed();
            return mismatch("expected setting " + field + " == " +
                            cmd.args[1] + " but was " + actual);
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
                return malformed("expect_setting " + field +
                                 " needs a number");
            }
            if (std::fabs(actual - expected) < 0.001f) return passed();
            return mismatch("expected setting " + field + " == " +
                            cmd.args[1] + " but was " +
                            std::to_string(actual));
        }
        if (field == "vsync_enabled" || field == "enable_postprocessing" ||
            field == "enable_lighting" || field == "isFullscreen") {
            bool actual = field == "vsync_enabled" ? d.vsync_enabled
                          : field == "enable_postprocessing"
                              ? d.enable_postprocessing
                              : field == "enable_lighting" ? d.enable_lighting
                                                           : d.isFullscreen;
            bool expected = cmd.args[1] == "true" || cmd.args[1] == "1";
            if (actual == expected) return passed();
            return mismatch("expected setting " + field + " == " +
                            cmd.args[1] + " but was " +
                            (actual ? "true" : "false"));
        }
        return malformed("unknown setting field '" + field + "'");
    }
    return malformed("unknown expect command '" + cmd.name + "'");
}

void handle_expect(testing::PendingE2ECommand& cmd) {
    EvalResult r = eval_expect(cmd);
    if (r.pass) {
        cmd.consume();
    } else if (r.malformed) {
        cmd.fail(r.message);
    } else {
        cmd.retry_with(r.message);
    }
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

fs::path script_dir() {
    if (current_script.empty()) return fs::path("tests/e2e");
    fs::path p(current_script);
    return p.has_parent_path() ? p.parent_path() : fs::path("tests/e2e");
}

struct WalkState {
    bool running = false;
    vec2 target{0.f, 0.f};
    float elapsed = 0.f;
    float timeout_s = 30.f;
    int line = -1;
};
WalkState walk_state;

int key_code(const std::string& name) {
    return afterhours::key_from_name(name);
}

void set_move_key(const char* name, bool down) {
    int code = key_code(name);
    if (code <= 0) return;
    if (down) {
        testing::input_injector::set_key_down(code);
    } else {
        testing::input_injector::set_key_up(code);
    }
}

void release_move_keys() {
    set_move_key("W", false);
    set_move_key("A", false);
    set_move_key("S", false);
    set_move_key("D", false);
}

void register_custom_commands(afterhours::SystemManager& sm) {
    testing::register_command(sm, "key",
                              [](testing::PendingE2ECommand& cmd) {
        if (cmd.args.empty()) {
            cmd.fail("key requires a key name");
            return;
        }
        int code = afterhours::key_from_name(cmd.args[0]);
        if (code == 0) {
            cmd.fail("unknown key '" + cmd.args[0] + "'");
            return;
        }
        testing::input_injector::set_key_down(code);
        log_info("e2e: key {} (code {})", cmd.args[0], code);
        cmd.consume();
    });


    auto expect = [](testing::PendingE2ECommand& cmd) { handle_expect(cmd); };
    testing::register_command(sm, "expect_menu_state", expect);
    testing::register_command(sm, "expect_game_state", expect);
    testing::register_command(sm, "expect_day", expect);
    testing::register_command(sm, "expect_phase", expect);
    testing::register_command(sm, "expect_entity", expect);
    testing::register_command(sm, "expect_order", expect);
    testing::register_command(sm, "expect_served", expect);
    testing::register_command(sm, "expect_money", expect);
    testing::register_command(sm, "expect_cart", expect);
    testing::register_command(sm, "expect_withdrawals", expect);
    testing::register_command(sm, "expect_save_slot", expect);
    testing::register_command(sm, "expect_furniture_near", expect);
    testing::register_command(sm, "expect_setting", expect);

    testing::register_command(
        sm, "walk_to", [](testing::PendingE2ECommand& cmd) {
            if (cmd.args.size() < 2) {
                cmd.fail("walk_to requires <x> <z> [timeout_s]");
                return;
            }
            if (!walk_state.running || walk_state.line != cmd.line_number) {
                try {
                    walk_state.target = vec2{std::stof(cmd.args[0]),
                                             std::stof(cmd.args[1])};
                } catch (...) {
                    cmd.fail("walk_to needs numeric coordinates");
                    return;
                }
                walk_state.timeout_s = 3600.f;
                if (cmd.args.size() >= 3) {
                    try {
                        walk_state.timeout_s = std::stof(cmd.args[2]);
                    } catch (...) {
                    }
                }
                walk_state.elapsed = 0.f;
                walk_state.line = cmd.line_number;
                walk_state.running = true;
            }
            Map* map = globals::world_map();
            if (!map || map->local_players_NOT_SERIALIZED.empty()) {
                walk_state.running = false;
                release_move_keys();
                cmd.fail("walk_to found no local player");
                return;
            }
            Entity& player = *map->local_players_NOT_SERIALIZED[0];
            vec2 pos = player.get<Transform>().as2();
            float dx = walk_state.target.x - pos.x;
            float dz = walk_state.target.y - pos.y;
            float dist = std::sqrt(dx * dx + dz * dz);
            if (dist < 0.45f) {
                release_move_keys();
                walk_state.running = false;
                log_info("e2e: walk_to arrived at ({:.1f}, {:.1f})", pos.x,
                         pos.y);
                cmd.consume();
                return;
            }
            float deadline = walk_state.timeout_s;
            if (cmd.wait_until_seconds > 0.f) {
                deadline = std::min(deadline, cmd.wait_until_seconds);
            }
            walk_state.elapsed += 1.f / 60.f;
            if (walk_state.elapsed >= deadline) {
                release_move_keys();
                walk_state.running = false;
                cmd.fail("walk_to timed out at (" + std::to_string(pos.x) +
                         ", " + std::to_string(pos.y) + ")");
                return;
            }
            set_move_key("D", dx > 0.3f);
            set_move_key("A", dx < -0.3f);
            set_move_key("S", dz > 0.3f);
            set_move_key("W", dz < -0.3f);
            cmd.retry_with("walking to (" + cmd.args[0] + ", " +
                           cmd.args[1] + ")");
        });

    testing::register_command(
        sm, "expect_debug", [](testing::PendingE2ECommand& cmd) {
            bool want = cmd.args.empty() || cmd.args[0] != "false";
            bool actual = globals::debug_ui_enabled();
            if (actual == want) {
                cmd.consume();
            } else {
                cmd.retry_with(std::string("expected debug_ui ") +
                               (want ? "on" : "off"));
            }
        });

    testing::register_command(
        sm, "ffwd_phase", [](testing::PendingE2ECommand& cmd) {
            static bool running = false;
            static float elapsed = 0.f;
            static int line = -1;
            if (cmd.args.empty()) {
                cmd.fail("ffwd_phase requires day|night");
                return;
            }
            std::string want = to_lower(cmd.args[0]);
            if (want != "day" && want != "night") {
                cmd.fail("ffwd_phase requires day|night");
                return;
            }
            if (!running || line != cmd.line_number) {
                running = true;
                elapsed = 0.f;
                line = cmd.line_number;
            }
            auto release = []() {
                int code = afterhours::key_from_name("R");
                if (code > 0) testing::input_injector::set_key_up(code);
            };
            OptEntity e =
                EQ().whereHasComponent<HasDayNightTimer>().gen_first();
            if (!e.valid()) {
                running = false;
                release();
                cmd.fail("ffwd_phase found no HasDayNightTimer entity");
                return;
            }
            const HasDayNightTimer& t = e->get<HasDayNightTimer>();
            bool is_day_phase = t.is_bar_closed();
            if ((want == "day" && is_day_phase) ||
                (want == "night" && !is_day_phase)) {
                running = false;
                release();
                cmd.consume();
                return;
            }
            float deadline = cmd.wait_until_seconds > 0.f
                                 ? cmd.wait_until_seconds * 0.95f
                                 : 300.f;
            elapsed += 1.f / 60.f;
            if (elapsed >= deadline) {
                running = false;
                release();
                cmd.fail("ffwd_phase timed out waiting for " + want);
                return;
            }
            int code = afterhours::key_from_name("R");
            if (code > 0) testing::input_injector::set_key_down(code);
            cmd.retry_with("fast-forwarding to " + want);
        });

    testing::register_command(
        sm, "save_slot", [](testing::PendingE2ECommand& cmd) {
            if (cmd.args.empty()) {
                cmd.fail("save_slot requires a slot number");
                return;
            }
            int slot = 0;
            try {
                slot = std::stoi(cmd.args[0]);
            } catch (...) {
                cmd.fail("save_slot needs a number");
                return;
            }
            if (!network_info || !network_info->client) {
                cmd.fail("save_slot requires an active network client");
                return;
            }
            network_info->client->send_save_slot_request(slot);
            log_info("e2e: queued save_slot {} request", slot);
            cmd.consume();
        });

    testing::register_command(
        sm, "set_setting", [](testing::PendingE2ECommand& cmd) {
            if (cmd.args.size() < 2) {
                cmd.fail("set_setting requires <field> <value>");
                return;
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
                    d.vsync_enabled =
                        cmd.args[1] == "true" || cmd.args[1] == "1";
                } else if (field == "enable_postprocessing") {
                    d.enable_postprocessing =
                        cmd.args[1] == "true" || cmd.args[1] == "1";
                } else if (field == "enable_lighting") {
                    d.enable_lighting =
                        cmd.args[1] == "true" || cmd.args[1] == "1";
                } else {
                    cmd.fail("unknown setting field '" + field + "'");
                    return;
                }
            } catch (...) {
                cmd.fail("bad value for setting '" + field + "'");
                return;
            }
            cmd.consume();
        });

    testing::register_command(sm, "save_settings",
                              [](testing::PendingE2ECommand& cmd) {
                                  Settings::get().write_save_file();
                                  cmd.consume();
                              });
    testing::register_command(sm, "reload_settings",
                              [](testing::PendingE2ECommand& cmd) {
                                  Settings::get().load_save_file();
                                  cmd.consume();
                              });
}

void register_diagnostics(afterhours::SystemManager& sm) {
    testing::register_command(sm, "whereami",
                              [](testing::PendingE2ECommand& cmd) {
        Map* map = globals::world_map();
        if (map && !map->local_players_NOT_SERIALIZED.empty()) {
            Entity& player = *map->local_players_NOT_SERIALIZED[0];
            vec2 pos = player.get<Transform>().as2();
            std::string facing = std::string(magic_enum::enum_name(
                player.get<Transform>().face_direction()));
            fmt::print(stderr,
                       "whereami: player at ({:.2f}, {:.2f}) facing {}\n",
                       pos.x, pos.y, facing);
        } else {
            log_warn("e2e: whereami found no local player");
        }
        cmd.consume();
    });

    testing::register_command(sm, "whereami_server",
                              [](testing::PendingE2ECommand& cmd) {
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
        cmd.consume();
    });

    testing::register_command(sm, "orders",
                              [](testing::PendingE2ECommand& cmd) {
        RefEntities ents = EQ().whereHasComponent<CanOrderDrink>().gen();
        for (Entity& e : ents) {
            const CanOrderDrink& cod = e.get<CanOrderDrink>();
            std::string ai = "?";
            if (e.has<IsAIControlled>()) {
                ai = std::string(
                    magic_enum::enum_name(e.get<IsAIControlled>().state));
            }
            fmt::print(stderr, "order: drink={} state={} ai={} drank={}\n",
                       magic_enum::enum_name(cod.order()),
                       magic_enum::enum_name(cod.order_state), ai,
                       cod.num_drinks_drank());
        }
        cmd.consume();
    });

    testing::register_command(sm, "holding",
                              [](testing::PendingE2ECommand& cmd) {
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
            cmd.consume();
            return;
        }
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
        cmd.consume();
    });

    testing::register_command(sm, "dump_entities",
                              [](testing::PendingE2ECommand& cmd) {
        auto dump_from = [](RefEntities ents) {
            for (Entity& e : ents) {
                vec2 pos = e.get<Transform>().as2();
                std::string label = "Unknown";
                for (size_t i = 0; i < magic_enum::enum_count<EntityType>();
                     i++) {
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
                            EntityType t =
                                magic_enum::enum_value<EntityType>(j);
                            if (item.hasTag(t)) {
                                held = std::string(magic_enum::enum_name(t));
                                break;
                            }
                        }
                        if (item.has<IsDrink>() &&
                            item.get<IsDrink>().underlying.has_value()) {
                            held += "(" +
                                    std::string(magic_enum::enum_name(
                                        item.get<IsDrink>()
                                            .underlying.value())) +
                                    ")";
                        }
                    }
                }
                fmt::print(stderr, "dump: {} at ({:.1f}, {:.1f}) holding {}\n",
                           label, pos.x, pos.y, held);
            }
        };
        dump_from(EQ().whereHasComponent<CanHoldItem>().gen());
        RefEntities ff = EQ().whereType(EntityType::FastForward).gen();
        for (Entity& e : ff) {
            vec2 pos = e.get<Transform>().as2();
            fmt::print(stderr, "dump: FastForward at ({:.1f}, {:.1f})\n",
                       pos.x, pos.y);
        }
        cmd.consume();
    });

    testing::register_command(sm, "dump_walls",
                              [](testing::PendingE2ECommand& cmd) {
        RefEntities ents = EQ().whereType(EntityType::Wall).gen();
        for (Entity& e : ents) {
            vec2 pos = e.get<Transform>().as2();
            fmt::print(stderr, "wall: ({:.1f}, {:.1f})\n", pos.x, pos.y);
        }
        cmd.consume();
    });

    testing::register_command(sm, "dump_store",
                              [](testing::PendingE2ECommand& cmd) {
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
        cmd.consume();
    });

    testing::register_command(sm, "dump_areas",
                              [](testing::PendingE2ECommand& cmd) {
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
        cmd.consume();
    });
}

}  // namespace

inline void init() {
    if (!E2E_ENABLED) return;

    if (!pack_ready) {
        register_custom_commands(pack);
        register_diagnostics(pack);
        testing::register_builtin_handlers(pack);
        testing::register_unknown_handler(pack);
        testing::register_cleanup(pack);
        pack_ready = true;
    }

    runner.set_timeout(1800.f);
    if (!E2E_RESULTS_JSON.empty()) {
        runner.set_results_path(E2E_RESULTS_JSON);
    }
    runner.set_screenshot_callback([](const std::string& name) {
        fs::path out = script_dir() / (name + ".png");
        bool ok = capture_png_to(out);
        if (ok) log_info("e2e: screenshot -> {}", out.string());
        return ok;
    });

    if (!E2E_DIR.empty()) {
        runner.load_scripts_from_directory(E2E_DIR);
        current_script = E2E_DIR;
    } else if (!E2E_SCRIPT.empty()) {
        runner.load_script(E2E_SCRIPT);
        current_script = E2E_SCRIPT;
    }

    if (!runner.has_commands()) {
        log_warn("E2E: no commands loaded, disabling");
        E2E_ENABLED = false;
        E2E_EXIT_CODE = 1;
        return;
    }

    active = true;
    log_info("E2E: runner loaded (dir={}, script={})", E2E_DIR, E2E_SCRIPT);
}

inline void update(float dt) {
    if (!active || finish_handled) return;

    afterhours::EntityCollection& isolated =
        afterhours::EntityHelper::get().default_collection;
    afterhours::EntityHelper::set_default_collection(&isolated);
    runner.tick(dt);
    pack.tick(isolated.get_entities_for_mod(), dt);
    isolated.cleanup();
    afterhours::EntityHelper::set_default_collection(
        &EntityHelper::get_client_collection());

    if (!runner.is_finished()) return;

    finish_handled = true;
    active = false;
    bool failed = runner.has_failed();
    E2E_EXIT_CODE = failed ? 1 : 0;

    if (failed) {
        std::string stem = "e2e";
        std::vector<testing::ScriptResult> results = runner.results();
        for (const testing::ScriptResult& r : results) {
            if (!r.passed) {
                stem = fs::path(r.path).stem().string();
                break;
            }
        }
        fs::path out = script_dir() / (stem + "_FAIL.png");
        if (capture_png_to(out)) {
            log_info("e2e: failure screenshot -> {}", out.string());
        } else {
            log_warn("e2e: failure screenshot failed for {}", out.string());
        }
    }

    runner.print_results();
    App::get().close();
}

}  // namespace e2e_manager
