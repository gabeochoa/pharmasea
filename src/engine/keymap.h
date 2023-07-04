
#pragma once

#include <tuple>

#include "raylib.h"
//
#include "files.h"
#include "statemanager.h"
#include "util.h"
//
#include "event.h"
#include "gamepad_axis_with_dir.h"
#include "log.h"
#include "singleton.h"

using raylib::GamepadAxis;
using raylib::GamepadButton;

// TODO this needs to not be in engine...
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
    PlayerDoWork,
    TargetForward,
    TargetBack,
    TargetLeft,
    TargetRight,
    //
    ToggleToPlanning,   // DEBUG ONLY
    ToggleToInRound,    // DEBUG ONLY
    ToggleDebug,        // DEBUG ONLY
    ToggleNetworkView,  // DEBUG ONLY
    ToggleLobby,        // DEBUG ONLY

    Last,  // This isnt real its just used to get the right number of bits
};

typedef std::bitset<InputName::Last> InputSet;
typedef std::tuple<InputSet, float> UserInput;
typedef std::vector<UserInput> UserInputs;

// TODO: had a bit of trouble trying to serialize "full map" which
// we need to do if we want to allow remapping of keys
typedef std::variant<int, GamepadAxisWithDir, GamepadButton> AnyInput;
typedef std::vector<AnyInput> AnyInputs;
typedef std::map<InputName, AnyInputs> LayerMapping;
typedef std::map<menu::State, LayerMapping> FullMap;

struct MouseInfo {
    vec2 pos;
    bool leftDown;
};

static const MouseInfo get_mouse_info() {
    return MouseInfo{
        .pos = ext::get_mouse_position(),
        .leftDown = raylib::IsMouseButtonDown(raylib::MOUSE_BUTTON_LEFT),
    };
}

enum class KeyMapInputRequestError {
    NO_VALID_INPUT = 0,
};

inline std::ostream& operator<<(std::ostream& os,
                                const KeyMapInputRequestError& info) {
    os << "KeyMapInputRequestError : " << magic_enum::enum_name(info);
    return os;
}

SINGLETON_FWD(KeyMap)
struct KeyMap {
    SINGLETON(KeyMap)

    static void forEachCharTyped(std::function<void(Event&)> cb) {
        int character = raylib::GetCharPressed();
        while (character) {
            CharPressedEvent* event = new CharPressedEvent(character, 0);
            cb(*event);
            delete event;
            character = raylib::GetCharPressed();
        }
    }

    void forEachInputInMap(std::function<void(Event&)> cb) const {
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

    [[nodiscard]] static float is_event(const menu::State& state,
                                        const InputName& name) {
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

    [[nodiscard]] static bool is_event_once_DO_NOT_USE(const menu::State& state,
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

    [[nodiscard]] static int get_key_code(const menu::State& state,
                                          const InputName& name) {
        const AnyInputs valid_inputs = KeyMap::get_valid_inputs(state, name);
        for (auto input : valid_inputs) {
            int r = std::visit(util::overloaded{[](int k) { return k; },
                                                [](auto&&) { return 0; }},
                               input);
            if (r) return r;
        }
        return raylib::KEY_NULL;
    }

    [[nodiscard]] static const tl::expected<GamepadAxisWithDir,
                                            KeyMapInputRequestError>
    get_axis(const menu::State& state, const InputName& name) {
        const AnyInputs valid_inputs = KeyMap::get_valid_inputs(state, name);
        for (auto input : valid_inputs) {
            auto r = std::visit(
                util::overloaded{
                    [](GamepadAxisWithDir ax) {
                        return std::optional<GamepadAxisWithDir>(ax);
                    },
                    [](auto&&) { return std::optional<GamepadAxisWithDir>(); }},
                input);
            if (r.has_value()) return r.value();
        }
        return tl::unexpected(KeyMapInputRequestError::NO_VALID_INPUT);
    }

    [[nodiscard]] static GamepadButton get_button(const menu::State& state,
                                                  const InputName& name) {
        const AnyInputs valid_inputs = KeyMap::get_valid_inputs(state, name);
        for (auto input : valid_inputs) {
            auto r = std::visit(
                util::overloaded{
                    [](GamepadButton button) { return button; },
                    [](auto&&) { return raylib::GAMEPAD_BUTTON_UNKNOWN; }},
                input);
            if (r != raylib::GAMEPAD_BUTTON_UNKNOWN) return r;
        }
        // TODO this is way too noisy to be valuable right now
        // log_warn("Couldn't find any button for {}", name);
        return raylib::GAMEPAD_BUTTON_UNKNOWN;
    }

    static bool does_layer_map_contain_key(const menu::State& state,
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

    [[nodiscard]] static bool does_layer_map_contain_button(
        const menu::State& state, GamepadButton button) {
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

    [[nodiscard]] static bool does_layer_map_contain_axis(
        const menu::State state, GamepadAxis axis) {
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

    [[nodiscard]] static const AnyInputs get_valid_inputs(
        const menu::State& state, const InputName& name) {
        return KeyMap::get().mapping[state][name];
    }

    [[nodiscard]] static const std::vector<int> get_valid_keys(
        const menu::State& state, const InputName& name) {
        const AnyInputs allInputs = get_valid_inputs(state, name);
        std::vector<int> keys;
        for (const auto& input : allInputs) {
            std::visit(util::overloaded{
                           //
                           [&](int keycode) { keys.push_back(keycode); },  //
                           [](GamepadAxisWithDir) {},                      //
                           [](GamepadButton) {},                           //
                           [](auto) {},                                    //
                       },
                       input);
        }
        return keys;
    }

   private:
    FullMap mapping;

    KeyMap() {
        load_controller_db();
        load_default_keys();
    }

    static void load_controller_db() {
        auto controller_db_fn = Files::get().game_controller_db();
        std::ifstream ifs(controller_db_fn);
        if (!ifs.is_open()) {
            log_warn("Failed to load game controller file {}",
                     controller_db_fn);
            return;
        }
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        ext::set_gamepad_mappings(buffer.str().c_str());
    }

    [[nodiscard]] LayerMapping& get_or_create_layer_map(
        const menu::State& state) {
        if (!this->mapping.contains(state)) {
            mapping[state] = LayerMapping();
        }
        return mapping[state];
    }

    [[nodiscard]] static float visit_key(int keycode) {
        return ext::is_key_pressed(keycode) ? 1.f : 0.f;
    }

    [[nodiscard]] static float visit_key_down(int keycode) {
        return ext::is_key_down(keycode) ? 1.f : 0.f;
    }

    [[nodiscard]] static float visit_axis(GamepadAxisWithDir axis_with_dir) {
        // Note: this one is a bit more complex because we have to check if you
        // are pushing in the right direction while also checking the magnitude
        float mvt = ext::get_gamepad_axis_movement(0, axis_with_dir.axis);
        // Note: The 0.25 is how big the deadzone is
        // TODO consider making the deadzone configurable?
        if (util::sgn(mvt) == axis_with_dir.dir && abs(mvt) > 0.25f) {
            return abs(mvt);
        }
        return 0.f;
    }

    [[nodiscard]] static float visit_button(GamepadButton button) {
        return IsGamepadButtonPressed(0, button) ? 1.f : 0.f;
    }

    [[nodiscard]] static float visit_button_down(GamepadButton button) {
        return IsGamepadButtonDown(0, button) ? 1.f : 0.f;
    }

    // TODO this needs to not be in engine...
    void load_game_keys() {
        LayerMapping& game_map =
            this->get_or_create_layer_map(menu::State::Game);
        game_map[InputName::PlayerForward] = {
            raylib::KEY_W,
            raylib::GAMEPAD_BUTTON_LEFT_FACE_UP,
            GamepadAxisWithDir{
                .axis = raylib::GAMEPAD_AXIS_LEFT_Y,
                .dir = -1,
            },
        };
        game_map[InputName::PlayerBack] = {
            raylib::KEY_S,
            raylib::GAMEPAD_BUTTON_LEFT_FACE_DOWN,
            GamepadAxisWithDir{
                .axis = raylib::GAMEPAD_AXIS_LEFT_Y,
                .dir = 1,
            },
        };
        game_map[InputName::PlayerLeft] = {
            raylib::KEY_A,
            raylib::GAMEPAD_BUTTON_LEFT_FACE_LEFT,
            GamepadAxisWithDir{
                .axis = raylib::GAMEPAD_AXIS_LEFT_X,
                .dir = -1,
            },
        };
        game_map[InputName::PlayerRight] = {
            raylib::KEY_D,
            raylib::GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
            GamepadAxisWithDir{
                .axis = raylib::GAMEPAD_AXIS_LEFT_X,
                .dir = 1,
            },
        };

        game_map[InputName::PlayerPickup] = {
            raylib::KEY_SPACE,
            raylib::GAMEPAD_BUTTON_RIGHT_FACE_DOWN,
        };

        game_map[InputName::PlayerRotateFurniture] = {
            raylib::KEY_R,
            raylib::GAMEPAD_BUTTON_RIGHT_FACE_LEFT,
        };

        game_map[InputName::PlayerDoWork] = {
            raylib::KEY_R,
            raylib::GAMEPAD_BUTTON_RIGHT_FACE_LEFT,
        };

        game_map[InputName::Pause] = {
            raylib::KEY_ESCAPE,
            raylib::GAMEPAD_BUTTON_MIDDLE_RIGHT,
        };

        game_map[InputName::TargetForward] = {raylib::KEY_UP};
        game_map[InputName::TargetBack] = {raylib::KEY_DOWN};
        game_map[InputName::TargetLeft] = {raylib::KEY_LEFT};
        game_map[InputName::TargetRight] = {raylib::KEY_RIGHT};

        game_map[InputName::ToggleToPlanning] = {
            raylib::KEY_P,
            raylib::GAMEPAD_BUTTON_MIDDLE_LEFT,
        };

        game_map[InputName::ToggleToInRound] = {
            raylib::KEY_O,
            raylib::GAMEPAD_BUTTON_MIDDLE_LEFT,
        };

        game_map[InputName::ToggleLobby] = {
            raylib::KEY_L,
        };

        game_map[InputName::ToggleDebug] = {
            raylib::KEY_BACKSLASH,
        };

        game_map[InputName::ToggleNetworkView] = {
            raylib::KEY_EQUAL,
        };
    }

    // TODO this could probably stay
    void load_ui_keys() {
        LayerMapping& ui_map = this->get_or_create_layer_map(menu::State::UI);
        ui_map[InputName::WidgetNext] = {
            raylib::KEY_TAB,
            raylib::GAMEPAD_BUTTON_LEFT_FACE_DOWN,
        };

        ui_map[InputName::WidgetBack] = {
            raylib::GAMEPAD_BUTTON_LEFT_FACE_UP,
        };
        ui_map[InputName::WidgetMod] = {raylib::KEY_LEFT_SHIFT};
        ui_map[InputName::WidgetBackspace] = {raylib::KEY_BACKSPACE};

#ifdef __APPLE__
        // For mac, paste is ⌘+v
        ui_map[InputName::WidgetCtrl] = {raylib::KEY_LEFT_SUPER};
#else
        // for windows ctrl+v
        ui_map[InputName::WidgetCtrl] = {raylib::KEY_LEFT_CONTROL};
#endif

        ui_map[InputName::WidgetPaste] = {raylib::KEY_V};

        ui_map[InputName::WidgetPress] = {
            raylib::KEY_ENTER, raylib::GAMEPAD_BUTTON_RIGHT_FACE_DOWN};

        ui_map[InputName::ValueUp] = {
            raylib::KEY_UP,
            raylib::GAMEPAD_BUTTON_LEFT_FACE_UP,
        };

        ui_map[InputName::ValueDown] = {
            raylib::KEY_DOWN,
            raylib::GAMEPAD_BUTTON_LEFT_FACE_DOWN,
        };

        ui_map[InputName::ValueLeft] = {
            raylib::KEY_LEFT,
            raylib::GAMEPAD_BUTTON_LEFT_FACE_LEFT,
            GamepadAxisWithDir{
                .axis = raylib::GAMEPAD_AXIS_LEFT_X,
                .dir = -1,
            },
        };
        ui_map[InputName::ValueRight] = {
            raylib::KEY_RIGHT,
            raylib::GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
            GamepadAxisWithDir{
                .axis = raylib::GAMEPAD_AXIS_LEFT_X,
                .dir = 1,
            },
        };

        ui_map[InputName::Pause] = {raylib::GAMEPAD_BUTTON_MIDDLE_RIGHT};

        LayerMapping& root_map =
            this->get_or_create_layer_map(menu::State::Root);
        for (auto kv : ui_map) {
            root_map[kv.first] = kv.second;
        }
    }

    void load_default_keys() {
        load_game_keys();
        load_ui_keys();
    }
};
