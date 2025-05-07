
#pragma once

#include <tuple>
#include <unordered_map>
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
using raylib::KeyboardKey;

extern float DEADZONE;

enum InputType {
    Keyboard,
    Gamepad,
    GamepadWithAxis,
};

// TODO this needs to not be in engine...
enum InputName {
    // Shared
    Pause,

    // UI State
    MenuBack,
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
    // If you add anything here, you need to
    // update num_per_col in layers/settings

    // Game State
    PlayerForward,
    PlayerBack,
    PlayerLeft,
    PlayerRight,
    PlayerPickup,
    PlayerRotateFurniture,
    PlayerDoWork,
    ShowRecipeBook,
    PlayerHandTruckInteract,
    RecipeNext,

    // TODO dont show any below in the settings page

    // Debug Only
    ToggleDebug,
    ToggleNetworkView,
    ToggleLobby,
    ToggleDebugSettings,
    SkipIngredientMatch,
};

using InputAmount = float;
using InputSet = std::array<InputAmount, magic_enum::enum_count<InputName>()>;
using UserInput = std::tuple<InputSet, float, float>;
using UserInputs = std::vector<UserInput>;

template<class Archive>
void serialize(Archive& archive, std::array<float, 28>& arr) {
    for (auto& element : arr) archive(element);
}

// Serialize for the specific tuple type
template<class Archive>
void serialize(Archive& archive,
               std::tuple<std::array<float, 28>, float, float>& input) {
    archive(std::get<0>(input), std::get<1>(input), std::get<2>(input));
}

template<class Archive>
void serialize(Archive& archive, InputName& iname) {
    archive(iname);
}

template<class Archive>
void serialize(Archive& archive, menu::State& state) {
    archive(state);
}

template<class Archive>
void serialize(Archive& archive, game::State& state) {
    archive(state);
}

// TODO: had a bit of trouble trying to serialize "full map" which
// we need to do if we want to allow remapping of keys
using AnyInput = std::variant<int, GamepadAxisWithDir, GamepadButton>;
using AnyInputs = std::vector<AnyInput>;
using LayerMapping = std::map<InputName, AnyInputs>;
using FullMap = std::map<menu::State, LayerMapping>;

struct AnyInputLess {
    std::size_t to_int(const AnyInput& key) const {
        if (std::holds_alternative<int>(key)) {
            return std::get<int>(key);
        }
        if (std::holds_alternative<GamepadAxisWithDir>(key)) {
            const auto axis_with_dir = std::get<GamepadAxisWithDir>(key);
            return 1000 +
                   100 *
                       (magic_enum::enum_index<GamepadAxis>(axis_with_dir.axis)
                            .value()) +
                   (int) (axis_with_dir.dir);
        }
        if (std::holds_alternative<GamepadButton>(key)) {
            const auto button = std::get<GamepadButton>(key);
            return 2000 + (size_t) magic_enum::enum_index<GamepadButton>(button)
                              .value();
        }
        return 0;
    }
    bool operator()(const AnyInput& lhs, const AnyInput& rhs) const {
        return to_int(lhs) < to_int(rhs);
    }
};

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

// TODO run into those raylib op overload errors when i tried to split this into
// a cpp file

SINGLETON_FWD(KeyMap)
struct KeyMap {
    SINGLETON(KeyMap)

    static void forEachCharTyped(const std::function<void(Event&)>& cb);
    void forEachInputInMap(const std::function<void(Event&)>& cb) const;
    [[nodiscard]] static float is_event(const menu::State& state,
                                        const InputName& name);
    [[nodiscard]] static bool is_event_once_DO_NOT_USE(const menu::State& state,
                                                       const InputName& name);
    [[nodiscard]] static int get_key_code(const menu::State& state,
                                          const InputName& name);

    [[nodiscard]] static const tl::expected<GamepadAxisWithDir,
                                            KeyMapInputRequestError>
    get_axis(const menu::State& state, const InputName& name);

    [[nodiscard]] static GamepadButton get_button(const menu::State& state,
                                                  const InputName& name);

    static bool does_layer_map_contain_key(const menu::State& state,
                                           int keycode);
    [[nodiscard]] static bool does_layer_map_contain_button(
        const menu::State& state, GamepadButton button);
    [[nodiscard]] static bool does_layer_map_contain_axis(
        const menu::State state, GamepadAxis axis);

    [[nodiscard]] static const AnyInputs get_valid_inputs(
        const menu::State& state, const InputName& name);

    [[nodiscard]] static const std::vector<int> get_valid_keys(
        const menu::State& state, const InputName& name);

    std::string name_for_key(int input);
    std::string name_for_button(GamepadButton input);
    std::string name_for_input(AnyInput input);

    std::string icon_for_input(AnyInput input);

   private:
    std::map<AnyInput, std::string, AnyInputLess> input_to_human_name;
    std::map<AnyInput, std::string, AnyInputLess> input_to_icon;
    FullMap mapping;

    KeyMap() {
        load_controller_db();
        load_default_keys();
    }

    LayerMapping& get_or_create_layer_map(const menu::State& state);

    static void load_controller_db();

    [[nodiscard]] static float visit_key(int keycode);

    [[nodiscard]] static float visit_key_down(int keycode);

    [[nodiscard]] static float visit_axis(GamepadAxisWithDir axis_with_dir);

    [[nodiscard]] static float visit_button(GamepadButton button);

    [[nodiscard]] static float visit_button_down(GamepadButton button);

    // TODO this needs to not be in engine...
    void load_game_keys();

    // TODO this could probably stay
    void load_ui_keys();

    void load_default_keys() {
        load_game_keys();
        load_ui_keys();
    }

    nlohmann::json anyinput_to_json(const AnyInput& key);
    AnyInput json_to_anyinput(const nlohmann::json& json);

   public:
    void set_mapping(menu::State state, InputName input_name, AnyInput input);
    nlohmann::json serializeFullMap();
    // Function to deserialize JSON to FullMap
    void deserializeFullMap(const nlohmann::json& serializedMap);
};
