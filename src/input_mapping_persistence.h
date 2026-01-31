#pragma once

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "game_actions.h"
#include "input_mapping_setup.h"
#include "afterhours/src/plugins/input_system.h"

namespace game {

// Convert AnyInput to JSON
inline nlohmann::json anyinput_to_json(const afterhours::input::AnyInput& input) {
    nlohmann::json j;
    if (input.index() == 0) {
        // KeyCode (int)
        j["type"] = "key";
        j["value"] = std::get<0>(input);
    } else if (input.index() == 1) {
        // GamepadAxisWithDir
        const auto& axis = std::get<1>(input);
        j["type"] = "axis";
        j["axis"] = static_cast<int>(axis.axis);
        j["dir"] = axis.dir;
    } else if (input.index() == 2) {
        // GamepadButton
        j["type"] = "button";
        j["value"] = static_cast<int>(std::get<2>(input));
    }
    return j;
}

// Convert JSON to AnyInput
inline afterhours::input::AnyInput json_to_anyinput(const nlohmann::json& j) {
    std::string type = j["type"];
    if (type == "key") {
        return static_cast<afterhours::input::KeyCode>(j["value"].get<int>());
    } else if (type == "axis") {
        return afterhours::input::GamepadAxisWithDir{
            static_cast<raylib::GamepadAxis>(j["axis"].get<int>()),
            j["dir"].get<int>()
        };
    } else if (type == "button") {
        return static_cast<afterhours::input::GamepadButton>(j["value"].get<int>());
    }
    return 0;  // Default to key 0
}

// Serialize layered mappings to JSON
inline nlohmann::json serialize_input_mappings(
    const afterhours::ProvidesLayeredInputMapping<menu::State>& mapper) {

    nlohmann::json root;
    for (const auto& [state, layer_map] : mapper.layers) {
        nlohmann::json layer_json;
        for (const auto& [action, inputs] : layer_map) {
            nlohmann::json inputs_json = nlohmann::json::array();
            for (const auto& input : inputs) {
                inputs_json.push_back(anyinput_to_json(input));
            }
            layer_json[std::to_string(action)] = inputs_json;
        }
        root[std::to_string(static_cast<int>(state))] = layer_json;
    }
    return root;
}

// Deserialize JSON to layered mappings
inline void deserialize_input_mappings(
    afterhours::ProvidesLayeredInputMapping<menu::State>& mapper,
    const nlohmann::json& data) {

    for (auto& [state_str, layer_json] : data.items()) {
        menu::State state = static_cast<menu::State>(std::stoi(state_str));
        for (auto& [action_str, inputs_json] : layer_json.items()) {
            int action = std::stoi(action_str);
            afterhours::input::ValidInputs inputs;
            for (const auto& input_json : inputs_json) {
                inputs.push_back(json_to_anyinput(input_json));
            }
            mapper.layers[state][action] = inputs;
        }
    }
}

// Save mappings to file
inline void save_input_mappings(
    const afterhours::ProvidesLayeredInputMapping<menu::State>& mapper,
    const std::filesystem::path& path) {

    nlohmann::json data = serialize_input_mappings(mapper);
    std::ofstream file(path);
    if (file.is_open()) {
        file << data.dump(2);
    }
}

// Load mappings from file
inline bool load_input_mappings(
    afterhours::ProvidesLayeredInputMapping<menu::State>& mapper,
    const std::filesystem::path& path) {

    if (!std::filesystem::exists(path)) {
        return false;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    nlohmann::json data = nlohmann::json::parse(file);
    deserialize_input_mappings(mapper, data);
    return true;
}

}  // namespace game
