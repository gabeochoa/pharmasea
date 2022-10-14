
#pragma once
#include "event.h"
#include "external_include.h"
#include "files.h"
#include "gamepad_axis_with_dir.h"
#include "menu.h"
#include "raylib.h"
#include "singleton.h"
#include "util.h"

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

    static float visit_key(int keycode) {
        if (IsKeyPressed(keycode)) {
            return 1.f;
        }
        return 0.f;
    }

    static float visit_axis(GamepadAxisWithDir axis_with_dir) {
        float mvt = GetGamepadAxisMovement(0, axis_with_dir.axis);
        if (util::sgn(mvt) == axis_with_dir.dir && abs(mvt) > 0.25f) {
            return abs(mvt);
        }
        return 0.f;
    }

    static float visit_button(GamepadButton button) {
        if (IsGamepadButtonPressed(0, button)) {
            return 1.f;
        }
        return 0.f;
    }

    void forEachInputInMap(std::function<void(Event&)> cb) {
        for (auto& fm_kv : mapping) {
            for (auto& lm_kv : fm_kv.second) {
                for (auto& input : lm_kv.second) {
                    std::visit(
                        util::overloaded{
                            [&](int keycode) {
                                if (visit_key(keycode) > 0.f) {
                                    KeyPressedEvent* event =
                                        new KeyPressedEvent(keycode, 0);
                                    cb(*event);
                                    delete event;
                                }
                            },
                            [&](GamepadAxisWithDir axis_with_dir) {
                                float res = visit_axis(axis_with_dir);
                                if (res > 0.f) {
                                    GamepadAxisMovedEvent* event =
                                        new GamepadAxisMovedEvent(
                                            GamepadAxisWithDir(
                                                {.axis = axis_with_dir.axis,
                                                 .dir = res}));

                                    cb(*event);
                                    delete event;
                                }
                            },
                            [&](GamepadButton button) {
                                if (visit_button(button) > 0.f) {
                                    GamepadButtonPressedEvent* event =
                                        new GamepadButtonPressedEvent(button);
                                    cb(*event);
                                    delete event;
                                }
                            },
                            [](auto) {}},
                        input);
                }
            }
        }
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
            GAMEPAD_BUTTON_RIGHT_FACE_LEFT,
        };

        game_map["Pause"] = {
            KEY_ESCAPE,
            GAMEPAD_BUTTON_MIDDLE_RIGHT,
        };

        game_map["Target Forward"] = {KEY_UP};
        game_map["Target Back"] = {KEY_DOWN};
        game_map["Target Left"] = {KEY_LEFT};
        game_map["Target Right"] = {KEY_RIGHT};
    }

    void load_ui_keys() {
        LayerMapping& game_map = this->get_or_create_layer_map(Menu::State::UI);
        game_map["Widget Next"] = {
            KEY_TAB,
            GAMEPAD_BUTTON_LEFT_FACE_DOWN,
        };
        game_map["Widget Back"] = {
            GAMEPAD_BUTTON_LEFT_FACE_UP,
        };
        game_map["Widget Mod"] = {KEY_LEFT_SHIFT};

        game_map["Widget Press"] = {KEY_ENTER, GAMEPAD_BUTTON_RIGHT_FACE_DOWN};

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
            int r = std::visit(util::overloaded{[](int k) { return k; },
                                                [](auto&&) { return 0; }},
                               input);
            if (r) return r;
        }
        return KEY_NULL;
    }

    static GamepadButton get_button(Menu::State state,
                                    const std::string& name) {
        return GAMEPAD_BUTTON_UNKNOWN;

        AnyInputs valid_inputs = KeyMap::get_valid_inputs(state, name);
        for (auto input : valid_inputs) {
            auto r = std::visit(
                util::overloaded{[](GamepadButton button) { return button; },
                                 [](auto&&) { return GAMEPAD_BUTTON_UNKNOWN; }},
                input);
            if (r != GAMEPAD_BUTTON_UNKNOWN) return r;
        }
        return GAMEPAD_BUTTON_UNKNOWN;
    }

    static AnyInputs get_valid_inputs(Menu::State state,
                                      const std::string& name) {
        return KeyMap::get().mapping[state][name];
    }

    static float is_event(Menu::State state, const std::string& name) {
        AnyInputs valid_inputs = KeyMap::get_valid_inputs(state, name);

        float value = 0.f;
        for (auto& input : valid_inputs) {
            value = fmax(
                value,
                std::visit(util::overloaded{
                               [](int keycode) { return visit_key(keycode); },
                               [](GamepadAxisWithDir axis_with_dir) {
                                   return visit_axis(axis_with_dir);
                               },
                               [](GamepadButton button) {
                                   return visit_button(button);
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
                util::overloaded{
                    [](int keycode) { return visit_key(keycode) > 0.f; },
                    [](GamepadAxisWithDir axis_with_dir) {
                        return visit_axis(axis_with_dir) > 0.f;
                    },
                    [](GamepadButton button) {
                        return visit_button(button) > 0.f;
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

    static bool does_layer_map_contain_button(Menu::State state,
                                              GamepadButton button) {
        // We dont even have this Layer Map
        if (!KeyMap::get().mapping.contains(state)) return false;
        LayerMapping layermap = KeyMap::get().mapping[state];
        for (auto pair : layermap) {
            const auto valid_inputs = pair.second;
            for (auto input : valid_inputs) {
                std::visit(util::overloaded{[&](GamepadButton butt) {
                                                if (butt == button) return true;
                                                return false;
                                            },
                                            [](auto&&) { return false; }},
                           input);
            }
        }
        return false;
    }
};
