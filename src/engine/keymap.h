
#pragma once

#include <tuple>

#include "raylib.h"
//
#include "../files.h"
#include "../menu.h"
#include "../util.h"
//
#include "event.h"
#include "gamepad_axis_with_dir.h"
#include "singleton.h"

enum InputName {
    // Shared
    Pause,

    // UI State
    WidgetNext,
    WidgetBack,
    WidgetMod,
    WidgetBackspace,
    WidgetCtrl,
    WidgetPaste,
    WidgetPress,
    ValueUp,
    ValueDown,
    ValueLeft,
    ValueRight,

    // Game State
    PlayerForward,
    PlayerBack,
    PlayerLeft,
    PlayerRight,
    PlayerPickup,
    PlayerRotateFurniture,
    TargetForward,
    TargetBack,
    TargetLeft,
    TargetRight,
    //
    TogglePlanning,  // DEBUG ONLY
    ToggleDebug,     // DEBUG ONLY
};

typedef std::tuple<Menu::State, InputName, float, float> UserInput;
typedef std::vector<UserInput> UserInputs;
//
typedef std::variant<int, GamepadAxisWithDir, GamepadButton> AnyInput;
typedef std::vector<AnyInput> AnyInputs;
typedef std::map<InputName, AnyInputs> LayerMapping;
typedef std::map<Menu::State, LayerMapping> FullMap;

struct MouseInfo {
    vec2 pos;
    bool leftDown;
};

static const MouseInfo get_mouse_info() {
    return MouseInfo{
        .pos = GetMousePosition(),
        .leftDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT),
    };
}

SINGLETON_FWD(KeyMap)
struct KeyMap {
    SINGLETON(KeyMap)

    void forEachCharTyped(std::function<void(Event&)> cb) {
        int character = GetCharPressed();
        while (character) {
            CharPressedEvent* event = new CharPressedEvent(character, 0);
            cb(*event);
            delete event;
            character = GetCharPressed();
        }
    }

    void forEachInputInMap(std::function<void(Event&)> cb) {
        for (const auto& fm_kv : mapping) {
            for (const auto& lm_kv : fm_kv.second) {
                for (const auto& input : lm_kv.second) {
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

    static float is_event(const Menu::State& state, const InputName& name) {
        const AnyInputs valid_inputs = KeyMap::get_valid_inputs(state, name);

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
                                        [](GamepadButton button) {
                                            return visit_button_down(button);
                                        },
                                        [](auto) {}},
                                    input));
        }
        return value;
    }

    static bool is_event_once_DO_NOT_USE(const Menu::State& state,
                                         const InputName& name) {
        const AnyInputs valid_inputs = KeyMap::get_valid_inputs(state, name);

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

    static int get_key_code(const Menu::State& state, const InputName& name) {
        const AnyInputs valid_inputs = KeyMap::get_valid_inputs(state, name);
        for (auto input : valid_inputs) {
            int r = std::visit(util::overloaded{[](int k) { return k; },
                                                [](auto&&) { return 0; }},
                               input);
            if (r) return r;
        }
        return KEY_NULL;
    }

    static const std::optional<GamepadAxisWithDir> get_axis(
        const Menu::State& state, const InputName& name) {
        const AnyInputs valid_inputs = KeyMap::get_valid_inputs(state, name);
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

    static GamepadButton get_button(const Menu::State& state,
                                    const InputName& name) {
        const AnyInputs valid_inputs = KeyMap::get_valid_inputs(state, name);
        for (auto input : valid_inputs) {
            auto r = std::visit(
                util::overloaded{[](GamepadButton button) { return button; },
                                 [](auto&&) { return GAMEPAD_BUTTON_UNKNOWN; }},
                input);
            if (r != GAMEPAD_BUTTON_UNKNOWN) return r;
        }
        // std::cout << "couldnt find any button for " << name << std::endl;
        return GAMEPAD_BUTTON_UNKNOWN;
    }

    static bool does_layer_map_contain_key(const Menu::State& state,
                                           int keycode) {
        // We dont even have this Layer Map
        if (!KeyMap::get().mapping.contains(state)) return false;
        const LayerMapping layermap = KeyMap::get().mapping[state];
        bool contains = false;
        for (const auto& pair : layermap) {
            const auto valid_inputs = pair.second;
            for (auto input : valid_inputs) {
                contains = std::visit(
                    util::overloaded{[&](int k) { return k == keycode; },
                                     [](auto&&) { return false; }},
                    input);
                if (contains) return true;
            }
        }
        return false;
    }

    static bool does_layer_map_contain_button(const Menu::State& state,
                                              GamepadButton button) {
        // We dont even have this Layer Map
        if (!KeyMap::get().mapping.contains(state)) return false;
        const LayerMapping layermap = KeyMap::get().mapping[state];
        bool contains = false;
        for (const auto& pair : layermap) {
            const auto valid_inputs = pair.second;
            for (auto input : valid_inputs) {
                contains = std::visit(
                    util::overloaded{
                        [&](GamepadButton butt) { return butt == button; },
                        [](auto&&) { return false; }},
                    input);
                if (contains) return true;
            }
        }
        return false;
    }

    static bool does_layer_map_contain_axis(const Menu::State state,
                                            GamepadAxis axis) {
        // We dont even have this Layer Map
        if (!KeyMap::get().mapping.contains(state)) return false;
        const LayerMapping layermap = KeyMap::get().mapping[state];
        bool contains = false;
        for (const auto& pair : layermap) {
            const auto valid_inputs = pair.second;
            for (const auto& input : valid_inputs) {
                contains = std::visit(
                    util::overloaded{
                        [&](GamepadAxisWithDir ax) { return ax.axis == axis; },
                        [](auto&&) { return false; }},
                    input);
                if (contains) return true;
            }
        }
        return false;
    }

   private:
    FullMap mapping;

    KeyMap() {
        // TODO migrate to using file-api to avoid this hardcoded path
        std::ifstream ifs(fs::path("./resources/gamecontrollerdb.txt"));
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        SetGamepadMappings(buffer.str().c_str());
        load_default_keys();
    }

    LayerMapping& get_or_create_layer_map(const Menu::State& state) {
        if (!this->mapping.contains(state)) {
            mapping[state] = LayerMapping();
        }
        return mapping[state];
    }

    static float visit_key(int keycode) {
        return IsKeyPressed(keycode) ? 1.f : 0.f;
    }

    static float visit_key_down(int keycode) {
        return IsKeyDown(keycode) ? 1.f : 0.f;
    }

    static float visit_axis(GamepadAxisWithDir axis_with_dir) {
        // Note: this one is a bit more complex because we have to check if you
        // are pushing in the right direction while also checking the magnitude
        float mvt = GetGamepadAxisMovement(0, axis_with_dir.axis);
        // Note: The 0.25 is how big the deadzone is
        // TODO consider making the deadzone configurable?
        if (util::sgn(mvt) == axis_with_dir.dir && abs(mvt) > 0.25f) {
            return abs(mvt);
        }
        return 0.f;
    }

    static float visit_button(GamepadButton button) {
        return IsGamepadButtonPressed(0, button) ? 1.f : 0.f;
    }

    static float visit_button_down(GamepadButton button) {
        return IsGamepadButtonDown(0, button) ? 1.f : 0.f;
    }

    void load_game_keys() {
        LayerMapping& game_map =
            this->get_or_create_layer_map(Menu::State::Game);
        game_map[InputName::PlayerForward] = {
            KEY_W,
            GAMEPAD_BUTTON_LEFT_FACE_UP,
            GamepadAxisWithDir{
                .axis = GAMEPAD_AXIS_LEFT_Y,
                .dir = -1,
            },
        };
        game_map[InputName::PlayerBack] = {
            KEY_S,
            GAMEPAD_BUTTON_LEFT_FACE_DOWN,
            GamepadAxisWithDir{
                .axis = GAMEPAD_AXIS_LEFT_Y,
                .dir = 1,
            },
        };
        game_map[InputName::PlayerLeft] = {
            KEY_A,
            GAMEPAD_BUTTON_LEFT_FACE_LEFT,
            GamepadAxisWithDir{
                .axis = GAMEPAD_AXIS_LEFT_X,
                .dir = -1,
            },
        };
        game_map[InputName::PlayerRight] = {
            KEY_D,
            GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
            GamepadAxisWithDir{
                .axis = GAMEPAD_AXIS_LEFT_X,
                .dir = 1,
            },
        };

        game_map[InputName::PlayerPickup] = {
            KEY_SPACE,
            GAMEPAD_BUTTON_RIGHT_FACE_DOWN,
        };

        game_map[InputName::PlayerRotateFurniture] = {
            KEY_R,
            GAMEPAD_BUTTON_RIGHT_FACE_LEFT,
        };

        game_map[InputName::Pause] = {
            KEY_ESCAPE,
            GAMEPAD_BUTTON_MIDDLE_RIGHT,
        };

        game_map[InputName::TargetForward] = {KEY_UP};
        game_map[InputName::TargetBack] = {KEY_DOWN};
        game_map[InputName::TargetLeft] = {KEY_LEFT};
        game_map[InputName::TargetRight] = {KEY_RIGHT};

        game_map[InputName::TogglePlanning] = {
            KEY_P,
            GAMEPAD_BUTTON_MIDDLE_LEFT,
        };

        game_map[InputName::ToggleDebug] = {
            KEY_BACKSLASH,
        };
    }

    void load_ui_keys() {
        LayerMapping& ui_map = this->get_or_create_layer_map(Menu::State::UI);
        ui_map[InputName::WidgetNext] = {
            KEY_TAB,
            GAMEPAD_BUTTON_LEFT_FACE_DOWN,
        };

        ui_map[InputName::WidgetBack] = {
            GAMEPAD_BUTTON_LEFT_FACE_UP,
        };
        ui_map[InputName::WidgetMod] = {KEY_LEFT_SHIFT};
        ui_map[InputName::WidgetBackspace] = {KEY_BACKSPACE};

#ifdef __APPLE__
        // For mac, paste is ⌘+v
        ui_map[InputName::WidgetCtrl] = {KEY_LEFT_SUPER};
#else
        // for windows ctrl+v
        ui_map[InputName::WidgetCtrl] = {KEY_LEFT_CONTROL};
#endif

        ui_map[InputName::WidgetPaste] = {KEY_V};

        ui_map[InputName::WidgetPress] = {KEY_ENTER,
                                          GAMEPAD_BUTTON_RIGHT_FACE_DOWN};

        ui_map[InputName::ValueUp] = {
            KEY_UP,
            GAMEPAD_BUTTON_LEFT_FACE_UP,
        };

        ui_map[InputName::ValueDown] = {
            KEY_DOWN,
            GAMEPAD_BUTTON_LEFT_FACE_DOWN,
        };

        ui_map[InputName::ValueLeft] = {
            KEY_LEFT,
            GAMEPAD_BUTTON_LEFT_FACE_LEFT,
            GamepadAxisWithDir{
                .axis = GAMEPAD_AXIS_LEFT_X,
                .dir = -1,
            },
        };
        ui_map[InputName::ValueRight] = {
            KEY_RIGHT,
            GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
            GamepadAxisWithDir{
                .axis = GAMEPAD_AXIS_LEFT_X,
                .dir = 1,
            },
        };

        ui_map[InputName::Pause] = {GAMEPAD_BUTTON_MIDDLE_RIGHT};

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

    static const AnyInputs get_valid_inputs(const Menu::State& state,
                                            const InputName& name) {
        return KeyMap::get().mapping[state][name];
    }
};
