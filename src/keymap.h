
#pragma once
#include "event.h"
#include "external_include.h"
#include "files.h"
#include "gamepad_axis_with_dir.h"
#include "menu.h"
#include "raylib.h"
#include "singleton.h"
#include "util.h"

typedef std::variant<int, GamepadAxisWithDir, raylib::GamepadButton> AnyInput;
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
        raylib::SetGamepadMappings(buffer.str().c_str());
        load_default_keys();
    }

    LayerMapping& get_or_create_layer_map(Menu::State state) {
        if (!this->mapping.contains(state)) {
            mapping[state] = LayerMapping();
        }
        return mapping[state];
    }

    static float visit_key(int keycode) {
        if (raylib::IsKeyPressed(keycode)) {
            return 1.f;
        }
        return 0.f;
    }

    static float visit_key_down(int keycode) {
        if (raylib::IsKeyDown(keycode)) {
            return 1.f;
        }
        return 0.f;
    }

    static float visit_axis(GamepadAxisWithDir axis_with_dir) {
        float mvt = raylib::GetGamepadAxisMovement(0, axis_with_dir.axis);
        if (util::sgn(mvt) == axis_with_dir.dir && abs(mvt) > 0.25f) {
            return abs(mvt);
        }
        return 0.f;
    }

    static float visit_button(raylib::GamepadButton button) {
        if (raylib::IsGamepadButtonPressed(0, button)) {
            return 1.f;
        }
        return 0.f;
    }

    static float visit_button_down(raylib::GamepadButton button) {
        if (raylib::IsGamepadButtonDown(0, button)) {
            return 1.f;
        }
        return 0.f;
    }

    void forEachCharTyped(std::function<void(Event&)> cb) {
        int character = raylib::GetCharPressed();
        while (character) {
            CharPressedEvent* event = new CharPressedEvent(character, 0);
            cb(*event);
            delete event;
            character = raylib::GetCharPressed();
        }
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
                            [&](raylib::GamepadButton button) {
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
            raylib::KEY_W,
            raylib::GAMEPAD_BUTTON_LEFT_FACE_UP,
            GamepadAxisWithDir{
                .axis = raylib::GAMEPAD_AXIS_LEFT_Y,
                .dir = -1,
            },
        };
        game_map["Player Back"] = {
            raylib::KEY_S,
            raylib::GAMEPAD_BUTTON_LEFT_FACE_DOWN,
            GamepadAxisWithDir{
                .axis = raylib::GAMEPAD_AXIS_LEFT_Y,
                .dir = 1,
            },
        };
        game_map["Player Left"] = {
            raylib::KEY_A,
            raylib::GAMEPAD_BUTTON_LEFT_FACE_LEFT,
            GamepadAxisWithDir{
                .axis = raylib::GAMEPAD_AXIS_LEFT_X,
                .dir = -1,
            },
        };
        game_map["Player Right"] = {
            raylib::KEY_D,
            raylib::GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
            GamepadAxisWithDir{
                .axis = raylib::GAMEPAD_AXIS_LEFT_X,
                .dir = 1,
            },
        };

        game_map["Player Pickup"] = {
            raylib::KEY_SPACE,
            raylib::GAMEPAD_BUTTON_RIGHT_FACE_DOWN,
        };

        game_map["Player Rotate Furniture"] = {
            raylib::KEY_R,
            raylib::GAMEPAD_BUTTON_RIGHT_FACE_LEFT,
        };

        game_map["Pause"] = {
            raylib::KEY_ESCAPE,
            raylib::GAMEPAD_BUTTON_MIDDLE_RIGHT,
        };

        game_map["Target Forward"] = {raylib::KEY_UP};
        game_map["Target Back"] = {raylib::KEY_DOWN};
        game_map["Target Left"] = {raylib::KEY_LEFT};
        game_map["Target Right"] = {raylib::KEY_RIGHT};

        game_map["Toggle Planning [Debug]"] = {
            raylib::KEY_P,
            raylib::GAMEPAD_BUTTON_MIDDLE_LEFT,
        };
    }

    void load_ui_keys() {
        LayerMapping& ui_map = this->get_or_create_layer_map(Menu::State::UI);
        ui_map["Widget Next"] = {
            raylib::KEY_TAB,
            raylib::GAMEPAD_BUTTON_LEFT_FACE_DOWN,
        };

        ui_map["Widget Back"] = {
            raylib::GAMEPAD_BUTTON_LEFT_FACE_UP,
        };
        ui_map["Widget Mod"] = {raylib::KEY_LEFT_SHIFT};
        ui_map["Widget Backspace"] = {raylib::KEY_BACKSPACE};

        ui_map["Widget Press"] = {raylib::KEY_ENTER,
                                  raylib::GAMEPAD_BUTTON_RIGHT_FACE_DOWN};

        ui_map["Value Up"] = {
            raylib::KEY_UP,
            raylib::GAMEPAD_BUTTON_LEFT_FACE_UP,
        };

        ui_map["Value Down"] = {
            raylib::KEY_DOWN,
            raylib::GAMEPAD_BUTTON_LEFT_FACE_DOWN,
        };

        ui_map["Value Left"] = {
            raylib::KEY_LEFT,
            raylib::GAMEPAD_BUTTON_LEFT_FACE_LEFT,
            GamepadAxisWithDir{
                .axis = raylib::GAMEPAD_AXIS_LEFT_X,
                .dir = -1,
            },
        };
        ui_map["Value Right"] = {
            raylib::KEY_RIGHT,
            raylib::GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
            GamepadAxisWithDir{
                .axis = raylib::GAMEPAD_AXIS_LEFT_X,
                .dir = 1,
            },
        };

        ui_map["Pause"] = {raylib::GAMEPAD_BUTTON_MIDDLE_RIGHT};

        LayerMapping& root_map =
            this->get_or_create_layer_map(Menu::State::Root);
        for (auto kv : ui_map) {
            root_map[kv.first] = kv.second;
        }
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
        return raylib::KEY_NULL;
    }

    static std::optional<GamepadAxisWithDir> get_axis(Menu::State state,
                                                      const std::string& name) {
        AnyInputs valid_inputs = KeyMap::get_valid_inputs(state, name);
        for (auto input : valid_inputs) {
            auto r = std::visit(
                util::overloaded{
                    [](GamepadAxisWithDir ax) {
                        return std::optional<GamepadAxisWithDir>(ax);
                    },
                    [](auto&&) { return std::optional<GamepadAxisWithDir>(); }},
                input);
            if (r.has_value()) return r;
        }
        return {};
    }

    static raylib::GamepadButton get_button(Menu::State state,
                                            const std::string& name) {
        AnyInputs valid_inputs = KeyMap::get_valid_inputs(state, name);
        for (auto input : valid_inputs) {
            auto r = std::visit(
                util::overloaded{
                    [](raylib::GamepadButton button) { return button; },
                    [](auto&&) { return raylib::GAMEPAD_BUTTON_UNKNOWN; }},
                input);
            if (r != raylib::GAMEPAD_BUTTON_UNKNOWN) return r;
        }
        // std::cout << "couldnt find any button for " << name << std::endl;
        return raylib::GAMEPAD_BUTTON_UNKNOWN;
    }

    static AnyInputs get_valid_inputs(Menu::State state,
                                      const std::string& name) {
        return KeyMap::get().mapping[state][name];
    }

    static float is_event(Menu::State state, const std::string& name) {
        AnyInputs valid_inputs = KeyMap::get_valid_inputs(state, name);

        float value = 0.f;
        for (auto& input : valid_inputs) {
            value = fmax(value,
                         std::visit(util::overloaded{
                                        [](int keycode) {
                                            return visit_key_down(keycode);
                                        },
                                        [](GamepadAxisWithDir axis_with_dir) {
                                            return visit_axis(axis_with_dir);
                                        },
                                        [](raylib::GamepadButton button) {
                                            return visit_button_down(button);
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
                    [](raylib::GamepadButton button) {
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
        bool contains = false;
        for (auto pair : layermap) {
            const auto valid_inputs = pair.second;
            for (auto input : valid_inputs) {
                contains =
                    std::visit(util::overloaded{[&](int k) {
                                                    if (k == keycode)
                                                        return true;
                                                    return false;
                                                },
                                                [](auto&&) { return false; }},
                               input);
                if (contains) return true;
            }
        }
        return false;
    }

    static bool does_layer_map_contain_button(Menu::State state,
                                              raylib::GamepadButton button) {
        // We dont even have this Layer Map
        if (!KeyMap::get().mapping.contains(state)) return false;
        LayerMapping layermap = KeyMap::get().mapping[state];
        bool contains = false;
        for (auto pair : layermap) {
            const auto valid_inputs = pair.second;
            for (auto input : valid_inputs) {
                contains = std::visit(
                    util::overloaded{[&](raylib::GamepadButton butt) {
                                         if (butt == button) return true;
                                         return false;
                                     },
                                     [](auto&&) { return false; }},
                    input);
                if (contains) return true;
            }
        }
        return false;
    }

    static bool does_layer_map_contain_axis(Menu::State state,
                                            raylib::GamepadAxis axis) {
        // We dont even have this Layer Map
        if (!KeyMap::get().mapping.contains(state)) return false;
        LayerMapping layermap = KeyMap::get().mapping[state];
        bool contains = false;
        for (auto pair : layermap) {
            const auto valid_inputs = pair.second;
            for (auto input : valid_inputs) {
                contains =
                    std::visit(util::overloaded{[&](GamepadAxisWithDir ax) {
                                                    if (ax.axis == axis)
                                                        return true;
                                                    return false;
                                                },
                                                [](auto&&) { return false; }},
                               input);
                if (contains) return true;
            }
        }
        return false;
    }
};
