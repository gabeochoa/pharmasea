
#include "keymap.h"

#include "graphics.h"

void KeyMap::forEachCharTyped(std::function<void(Event&)> cb) {
    int character = raylib::GetCharPressed();
    while (character) {
        CharPressedEvent* event = new CharPressedEvent(character, 0);
        cb(*event);
        delete event;
        character = raylib::GetCharPressed();
    }
}

void KeyMap::forEachInputInMap(std::function<void(Event&)> cb) const {
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

float KeyMap::is_event(const menu::State& state, const InputName& name) {
    const AnyInputs valid_inputs = KeyMap::get_valid_inputs(state, name);

    float value = 0.f;
    for (auto& input : valid_inputs) {
        value = fmax(
            value,
            std::visit(util::overloaded{
                           [](int keycode) { return visit_key_down(keycode); },
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

bool KeyMap::is_event_once_DO_NOT_USE(const menu::State& state,
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
                [](GamepadButton button) { return visit_button(button) > 0.f; },
                [](auto) {}},
            input);
    }
    return matches_named_event;
}

int KeyMap::get_key_code(const menu::State& state, const InputName& name) {
    const AnyInputs valid_inputs = KeyMap::get_valid_inputs(state, name);
    for (auto input : valid_inputs) {
        int r = std::visit(
            util::overloaded{[](int k) { return k; }, [](auto&&) { return 0; }},
            input);
        if (r) return r;
    }
    return raylib::KEY_NULL;
}

const tl::expected<GamepadAxisWithDir, KeyMapInputRequestError>
KeyMap::get_axis(const menu::State& state, const InputName& name) {
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

GamepadButton KeyMap::get_button(const menu::State& state,
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

bool KeyMap::does_layer_map_contain_key(const menu::State& state, int keycode) {
    // We dont even have this Layer Map
    if (!KeyMap::get().mapping.contains(state)) return false;
    const LayerMapping layermap = KeyMap::get().mapping[state];
    bool contains = false;
    for (const auto& pair : layermap) {
        const auto valid_inputs = pair.second;
        for (auto input : valid_inputs) {
            contains =
                std::visit(util::overloaded{[&](int k) { return k == keycode; },
                                            [](auto&&) { return false; }},
                           input);
            if (contains) return true;
        }
    }
    return false;
}

bool KeyMap::does_layer_map_contain_button(const menu::State& state,
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

bool KeyMap::does_layer_map_contain_axis(const menu::State state,
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

const AnyInputs KeyMap::get_valid_inputs(const menu::State& state,
                                         const InputName& name) {
    return KeyMap::get().mapping[state][name];
}

const std::vector<int> KeyMap::get_valid_keys(const menu::State& state,
                                              const InputName& name) {
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

std::string KeyMap::name_for_key(int input) {
    KeyboardKey key = magic_enum::enum_cast<KeyboardKey>(input).value();
    return std::string(magic_enum::enum_name(key));
}

std::string KeyMap::name_for_button(GamepadButton input) {
    switch (input) {
        case raylib::GAMEPAD_BUTTON_LEFT_FACE_UP:
            return "D-Pad Up";
        case raylib::GAMEPAD_BUTTON_LEFT_FACE_RIGHT:
            return "D-Pad Right";
        case raylib::GAMEPAD_BUTTON_LEFT_FACE_DOWN:
            return "D-Pad Down";
        case raylib::GAMEPAD_BUTTON_LEFT_FACE_LEFT:
            return "D-Pad Left";
        case raylib::GAMEPAD_BUTTON_RIGHT_FACE_UP:
            return "PS3: Triangle, Xbox: Y";
        case raylib::GAMEPAD_BUTTON_RIGHT_FACE_RIGHT:
            return "PS3: Square, Xbox: X";
        case raylib::GAMEPAD_BUTTON_RIGHT_FACE_DOWN:
            return "PS3: Cross, Xbox: A";
        case raylib::GAMEPAD_BUTTON_RIGHT_FACE_LEFT:
            return "PS3: Circle, Xbox: B";
        case raylib::GAMEPAD_BUTTON_MIDDLE_LEFT:
            return "Select";
        case raylib::GAMEPAD_BUTTON_MIDDLE:
            return "PS3: PS, Xbox: XBOX";
        case raylib::GAMEPAD_BUTTON_MIDDLE_RIGHT:
            return "Start";
        default:
            return std::string(magic_enum::enum_name(input));
    }
}

std::string KeyMap::name_for_input(AnyInput input) {
    if (!input_to_human_name.contains(input)) {
        std::string value =
            std::visit(util::overloaded{
                           //
                           [this](int keycode) -> std::string {
                               return name_for_key(keycode);
                           },
                           [](GamepadAxisWithDir) -> std::string {
                               return std::string("TODO AXIS?");
                           },
                           [this](GamepadButton button) -> std::string {
                               return name_for_button(button);
                           },
                           [](auto) {},
                       },
                       input);

        input_to_human_name[input] = value;
    }
    return input_to_human_name[input];
}

void KeyMap::load_controller_db() {
    auto controller_db_fn = Files::get().game_controller_db();
    std::ifstream ifs(controller_db_fn);
    if (!ifs.is_open()) {
        log_warn("Failed to load game controller file {}", controller_db_fn);
        return;
    }
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    ext::set_gamepad_mappings(buffer.str().c_str());
}

LayerMapping& KeyMap::get_or_create_layer_map(const menu::State& state) {
    if (!this->mapping.contains(state)) {
        mapping[state] = LayerMapping();
    }
    return mapping[state];
}

float KeyMap::visit_key(int keycode) {
    return ext::is_key_pressed(keycode) ? 1.f : 0.f;
}

float KeyMap::visit_key_down(int keycode) {
    return ext::is_key_down(keycode) ? 1.f : 0.f;
}

float KeyMap::visit_axis(GamepadAxisWithDir axis_with_dir) {
    // Note: this one is a bit more complex because we have to check if you
    // are pushing in the right direction while also checking the magnitude
    float mvt = ext::get_gamepad_axis_movement(0, axis_with_dir.axis);
    // Note: The 0.25 is how big the deadzone is
    // TODO consider making the deadzone configurable?
    if (util::sgn(mvt) == axis_with_dir.dir && abs(mvt) > DEADZONE) {
        return abs(mvt);
    }
    return 0.f;
}

float KeyMap::visit_button(GamepadButton button) {
    return IsGamepadButtonPressed(0, button) ? 1.f : 0.f;
}

float KeyMap::visit_button_down(GamepadButton button) {
    return IsGamepadButtonDown(0, button) ? 1.f : 0.f;
}

// TODO this needs to not be in engine...
void KeyMap::load_game_keys() {
    LayerMapping& game_map = this->get_or_create_layer_map(menu::State::Game);
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

    game_map[InputName::ShowRecipeBook] = {
        raylib::KEY_TAB,
        raylib::GAMEPAD_BUTTON_RIGHT_FACE_UP,
    };

    game_map[InputName::Pause] = {
        raylib::KEY_ESCAPE,
        raylib::GAMEPAD_BUTTON_MIDDLE_RIGHT,
    };

    game_map[InputName::ToggleLobby] = {
        raylib::KEY_L,
    };

    game_map[InputName::ToggleDebug] = {
        raylib::KEY_BACKSLASH,
    };

    game_map[InputName::ToggleNetworkView] = {
        raylib::KEY_EQUAL,
        raylib::GAMEPAD_BUTTON_MIDDLE_LEFT,
    };

    game_map[InputName::ToggleDebugSettings] = {
        raylib::KEY_GRAVE,
    };

    game_map[InputName::SkipIngredientMatch] = {
        raylib::KEY_I,
    };

    game_map[InputName::RecipeNext] = {
        raylib::KEY_RIGHT,
        raylib::GAMEPAD_BUTTON_RIGHT_FACE_RIGHT,
    };
}

// TODO this could probably stay
void KeyMap::load_ui_keys() {
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

    ui_map[InputName::WidgetCtrl] = {// For mac, paste is ⌘+v
                                     raylib::KEY_LEFT_SUPER,
                                     // for windows ctrl+v
                                     raylib::KEY_LEFT_CONTROL};

    ui_map[InputName::WidgetPaste] = {raylib::KEY_V};

    ui_map[InputName::WidgetPress] = {raylib::KEY_ENTER,
                                      raylib::GAMEPAD_BUTTON_RIGHT_FACE_DOWN};

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
    ui_map[InputName::MenuBack] = {raylib::GAMEPAD_BUTTON_RIGHT_FACE_RIGHT};

    LayerMapping& root_map = this->get_or_create_layer_map(menu::State::Root);
    for (auto kv : ui_map) {
        root_map[kv.first] = kv.second;
    }
}

nlohmann::json KeyMap::anyinput_to_json(const AnyInput& key) {
    if (std::holds_alternative<int>(key)) {
        nlohmann::json data = {
            //
            {"type", InputType::Keyboard},
            {"value", std::get<int>(key)}
            //
        };
        return data;
    }
    if (std::holds_alternative<GamepadAxisWithDir>(key)) {
        GamepadAxisWithDir gwa = std::get<GamepadAxisWithDir>(key);
        nlohmann::json data = {
            {"type", InputType::GamepadWithAxis},  //
            {
                "value",
                {
                    //
                    {"axis", magic_enum::enum_name<GamepadAxis>(gwa.axis)},
                    {"dir", gwa.dir}
                    //
                }
                //
            }};
        return data;
    }
    if (std::holds_alternative<GamepadButton>(key)) {
        auto button = std::get<GamepadButton>(key);
        nlohmann::json data = {
            //
            {"type", InputType::Gamepad},  //
            {"value", magic_enum::enum_name<GamepadButton>(button)}
            //
        };
        return data;
    }
    return {};
}

AnyInput KeyMap::json_to_anyinput(const nlohmann::json& json) {
    int type_as_int = json["type"].get<int>();
    InputType type = magic_enum::enum_value<InputType>(type_as_int);

    auto jvalue = json["value"];

    switch (type) {
        case InputType::Keyboard:
            return jvalue.get<int>();
        case InputType::Gamepad:
            return magic_enum::enum_cast<GamepadButton>(
                       jvalue.get<std::string>())
                .value();
        case InputType::GamepadWithAxis:
            return GamepadAxisWithDir{
                .axis = magic_enum::enum_cast<GamepadAxis>(
                            jvalue["axis"].get<std::string>())
                            .value(),
                .dir = jvalue["dir"].get<float>()};
    }
    log_warn("couldnt find matching AnyInput for json");
    return jvalue.get<int>();
}

void KeyMap::set_mapping(menu::State state, InputName input_name,
                         AnyInput input) {
    // For now we are just going to clear the old one and write the new
    // one...

    LayerMapping& state_map = this->get_or_create_layer_map(state);

    auto& input_list = state_map[input_name];

    // TODO only change the one that matches our input type
    input_list.clear();

    //

    input_list.push_back(input);
}

nlohmann::json KeyMap::serializeFullMap() {
    nlohmann::json serializedMap;
    for (const auto& [state, layerMapping] : mapping) {
        nlohmann::json stateJson;
        for (const auto& [inputName, anyInputs] : layerMapping) {
            nlohmann::json inputArray;
            for (const AnyInput& anyInput : anyInputs) {
                auto val = anyinput_to_json(anyInput);
                if (val.empty()) continue;
                inputArray.push_back(val);
            }

            if (inputArray.empty()) continue;
            stateJson[magic_enum::enum_name<InputName>(inputName)] = inputArray;
        }

        if (stateJson.empty()) continue;
        serializedMap[magic_enum::enum_name<menu::State>(state)] = stateJson;
    }
    return serializedMap;
}

// Function to deserialize JSON to FullMap
void KeyMap::deserializeFullMap(const nlohmann::json& serializedMap) {
    for (auto it = serializedMap.begin(); it != serializedMap.end(); ++it) {
        menu::State state =
            magic_enum::enum_cast<menu::State>(it.key()).value();
        LayerMapping layerMapping;
        for (const auto& [inputNameStr, inputArray] : it.value().items()) {
            InputName inputName =
                magic_enum::enum_cast<InputName>(inputNameStr).value();

            AnyInputs anyInputs;
            for (const auto& anyInput : inputArray) {
                anyInputs.push_back(json_to_anyinput(anyInput));
            }

            layerMapping[inputName] = anyInputs;
        }
        mapping[state] = layerMapping;
    }
}
