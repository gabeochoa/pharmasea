
#include "keymap.h"

#include "graphics.h"

const AnyInputs KeyMap::get_valid_inputs(const menu::State& state,
                                         const InputName& name) {
    return KeyMap::get().mapping[state][name];
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
        raylib::KEY_F,
        raylib::GAMEPAD_BUTTON_RIGHT_FACE_RIGHT,
    };

    game_map[InputName::PlayerDoWork] = {
        raylib::KEY_R,
        raylib::GAMEPAD_BUTTON_RIGHT_FACE_LEFT,
    };

    game_map[InputName::PlayerHandTruckInteract] = {
        raylib::KEY_C,
        raylib::GAMEPAD_BUTTON_RIGHT_FACE_UP,
    };

    game_map[InputName::ShowRecipeBook] = {
        raylib::KEY_TAB,
        raylib::GAMEPAD_BUTTON_MIDDLE_LEFT,
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

    // TODO remove this from the game
    game_map[InputName::ToggleNetworkView] = {
        raylib::KEY_EQUAL,
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
