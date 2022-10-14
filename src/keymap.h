
#pragma once

#include "external_include.h"
#include "files.h"
#include "menu.h"
#include "raylib.h"
#include "singleton.h"
#include "util.h"

struct GamepadAxisWithDir {
    GamepadAxis axis;
    int dir = -1;
};

typedef std::variant<int, GamepadAxisWithDir, GamepadButton> AnyInput;
typedef std::vector<AnyInput> AnyInputs;
typedef std::map<std::string, AnyInputs> LayerMapping;
typedef std::map<Menu::State, LayerMapping> FullMap;

SINGLETON_FWD(KeyMap)
struct KeyMap {
    SINGLETON(KeyMap)

    FullMap mapping;

    KeyMap() {
        std::ifstream ifs(fs::path("./resources/gamecontrollerdb.txt"));
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        SetGamepadMappings(buffer.str().c_str());
        load_default_keys();
    }

    LayerMapping& get_or_create_layer_map(Menu::State state) {
        if (!this->mapping.contains(state)) {
            mapping[state] = LayerMapping();
        }
        return mapping[state];
    }

    void load_game_keys() {
        LayerMapping& game_map =
            this->get_or_create_layer_map(Menu::State::Game);
        game_map["Player Forward"] = {
            KEY_W,
            GAMEPAD_BUTTON_LEFT_FACE_UP,
            GamepadAxisWithDir{
                .axis = GAMEPAD_AXIS_LEFT_Y,
                .dir = -1,
            },
        };
        game_map["Player Back"] = {
            KEY_S,
            GAMEPAD_BUTTON_LEFT_FACE_DOWN,
            GamepadAxisWithDir{
                .axis = GAMEPAD_AXIS_LEFT_Y,
                .dir = 1,
            },
        };
        game_map["Player Left"] = {
            KEY_A,
            GAMEPAD_BUTTON_LEFT_FACE_LEFT,
            GamepadAxisWithDir{
                .axis = GAMEPAD_AXIS_LEFT_X,
                .dir = -1,
            },
        };
        game_map["Player Right"] = {
            KEY_D,
            GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
            GamepadAxisWithDir{
                .axis = GAMEPAD_AXIS_LEFT_X,
                .dir = 1,
            },
        };

        game_map["Player Pickup"] = {
            KEY_SPACE,
        };

        game_map["Target Forward"] = {KEY_UP};
        game_map["Target Back"] = {KEY_DOWN};
        game_map["Target Left"] = {KEY_LEFT};
        game_map["Target Right"] = {KEY_RIGHT};
    }

    void load_ui_keys() {
        LayerMapping& game_map = this->get_or_create_layer_map(Menu::State::UI);
        game_map["Widget Next"] = {KEY_TAB};
        game_map["Widget Mod"] = {KEY_LEFT_SHIFT};

        game_map["Widget Press"] = {KEY_ENTER};

        game_map["Widget Value Up"] = {KEY_UP};
        game_map["Widget Value Down"] = {KEY_DOWN};
    }

    void load_default_keys() {
        load_game_keys();
        load_ui_keys();
    }

    static int get_key_code(Menu::State state, const std::string& name) {
        AnyInputs valid_inputs = KeyMap::get_valid_inputs(state, name);
        for (auto input : valid_inputs) {
            int r = std::visit(util::overloaded{[&](int k) { return k; },
                                                [](auto&&) { return 0; }},
                               input);
            if (r) return r;
        }
        return KEY_NULL;
    }

    static AnyInputs get_valid_inputs(Menu::State state,
                                      const std::string& name) {
        return KeyMap::get().mapping[state][name];
    }

    static float is_event(Menu::State state, const std::string& name) {
        AnyInputs valid_inputs = KeyMap::get_valid_inputs(state, name);

        float value = 0.f;
        for (auto& input : valid_inputs) {
            value = fmax(value, std::visit(
                util::overloaded{[](int keycode) {
                                     if (IsKeyDown(keycode)) {
                                         return 1.f;
                                     }
                                     return 0.f;
                                 },
                                 [](GamepadAxisWithDir axis_with_dir) {
                                     float mvt = GetGamepadAxisMovement(
                                         0, axis_with_dir.axis);
                                     if (util::sgn(mvt) == axis_with_dir.dir && abs(mvt) > 0.25f) {
                                         return abs(mvt);
                                     }
                                     return 0.f;
                                 },
                                 [](GamepadButton button) {
                                     if (IsGamepadButtonDown(0, button)) {
                                         return 1.f;
                                     }
                                     return 0.f;
                                 },
                                 [](auto) {}},
                input));
        }
        return value;
    }

    static bool is_event_once_DO_NOT_USE(Menu::State state,
                                         const std::string& name) {
        AnyInputs valid_inputs = KeyMap::get_valid_inputs(state, name);

        bool matches_named_event = false;
        for (auto& input : valid_inputs) {
            matches_named_event |= std::visit(
                util::overloaded{[](int keycode) {
                                     if (IsKeyPressed(keycode)) {
                                         return true;
                                     }
                                     return false;
                                 },
                                 [](GamepadAxisWithDir axis_with_dir) {
                                     float mvt = GetGamepadAxisMovement(
                                         0, axis_with_dir.axis);
                                     if (util::sgn(mvt) == axis_with_dir.dir) {
                                         return true;
                                     }
                                     return false;
                                 },
                                 [](GamepadButton button) {
                                     if (IsGamepadButtonPressed(0, button)) {
                                         return true;
                                     }
                                     return false;
                                 },
                                 [](auto) {}},
                input);
        }
        return matches_named_event;
    }

    static bool does_layer_map_contain_key(Menu::State state, int keycode) {
        // We dont even have this Layer Map
        if (!KeyMap::get().mapping.contains(state)) return false;
        LayerMapping layermap = KeyMap::get().mapping[state];
        for (auto pair : layermap) {
            const auto valid_inputs = pair.second;
            for (auto input : valid_inputs) {
                std::visit(util::overloaded{[&](int k) {
                                                if (k == keycode) return true;
                                                return false;
                                            },
                                            [](auto&&) { return false; }},
                           input);
            }
        }
        return false;
    }
};
