#pragma once

#include "afterhours/src/plugins/input_system.h"
#include "engine/statemanager.h"
#include "game_actions.h"

namespace game {

using LayerMapping = std::map<int, afterhours::input::ValidInputs>;
using AllLayerMappings = std::map<menu::State, LayerMapping>;

inline AllLayerMappings create_default_layered_mapping() {
    using GamepadAxisWithDir = afterhours::input::GamepadAxisWithDir;
    using namespace raylib;

    AllLayerMappings mappings;

    // ========== Game State ==========
    mappings[menu::State::Game] = {
        // Movement - match existing bindings
        {GameAction::PlayerForward,
         {KEY_W, GAMEPAD_BUTTON_LEFT_FACE_UP,
          GamepadAxisWithDir{GAMEPAD_AXIS_LEFT_Y, -1}}},
        {GameAction::PlayerBack,
         {KEY_S, GAMEPAD_BUTTON_LEFT_FACE_DOWN,
          GamepadAxisWithDir{GAMEPAD_AXIS_LEFT_Y, 1}}},
        {GameAction::PlayerLeft,
         {KEY_A, GAMEPAD_BUTTON_LEFT_FACE_LEFT,
          GamepadAxisWithDir{GAMEPAD_AXIS_LEFT_X, -1}}},
        {GameAction::PlayerRight,
         {KEY_D, GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
          GamepadAxisWithDir{GAMEPAD_AXIS_LEFT_X, 1}}},

        // Player Actions
        {GameAction::PlayerPickup, {KEY_SPACE, GAMEPAD_BUTTON_RIGHT_FACE_DOWN}},
        {GameAction::PlayerRotateFurniture,
         {KEY_F, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT}},
        {GameAction::PlayerDoWork, {KEY_R, GAMEPAD_BUTTON_RIGHT_FACE_LEFT}},
        {GameAction::PlayerHandTruckInteract,
         {KEY_C, GAMEPAD_BUTTON_RIGHT_FACE_UP}},

        // Game UI
        {GameAction::Pause, {KEY_ESCAPE, GAMEPAD_BUTTON_MIDDLE_RIGHT}},
        {GameAction::ShowRecipeBook, {KEY_TAB, GAMEPAD_BUTTON_MIDDLE_LEFT}},
        {GameAction::RecipeNext, {KEY_RIGHT, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT}},

        // Debug
        {GameAction::ToggleDebug, {KEY_BACKSLASH}},
        {GameAction::ToggleNetworkView, {KEY_EQUAL}},
        {GameAction::ToggleLobby, {KEY_L}},
        {GameAction::ToggleDebugSettings, {KEY_GRAVE}},
        {GameAction::SkipIngredientMatch, {KEY_I}},
    };

    // ========== UI State ==========
    LayerMapping ui_mapping = {
        // Navigation
        {GameAction::WidgetNext, {KEY_TAB, GAMEPAD_BUTTON_LEFT_FACE_DOWN}},
        {GameAction::WidgetBack, {GAMEPAD_BUTTON_LEFT_FACE_UP}},
        {GameAction::WidgetPress, {KEY_ENTER, GAMEPAD_BUTTON_RIGHT_FACE_DOWN}},
        {GameAction::MenuBack, {GAMEPAD_BUTTON_RIGHT_FACE_RIGHT}},
        {GameAction::Pause, {GAMEPAD_BUTTON_MIDDLE_RIGHT}},

        // Value adjustment
        {GameAction::ValueUp, {KEY_UP, GAMEPAD_BUTTON_LEFT_FACE_UP}},
        {GameAction::ValueDown, {KEY_DOWN, GAMEPAD_BUTTON_LEFT_FACE_DOWN}},
        {GameAction::ValueLeft,
         {KEY_LEFT, GAMEPAD_BUTTON_LEFT_FACE_LEFT,
          GamepadAxisWithDir{GAMEPAD_AXIS_LEFT_X, -1}}},
        {GameAction::ValueRight,
         {KEY_RIGHT, GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
          GamepadAxisWithDir{GAMEPAD_AXIS_LEFT_X, 1}}},

        // Text input modifiers
        {GameAction::WidgetMod, {KEY_LEFT_SHIFT}},
        {GameAction::WidgetBackspace, {KEY_BACKSPACE}},
        {GameAction::WidgetCtrl, {KEY_LEFT_SUPER, KEY_LEFT_CONTROL}},
        {GameAction::WidgetPaste, {KEY_V}},
    };

    // Apply UI mapping to menu states
    mappings[menu::State::UI] = ui_mapping;
    mappings[menu::State::Root] = ui_mapping;
    mappings[menu::State::Settings] = ui_mapping;
    mappings[menu::State::Network] = ui_mapping;
    mappings[menu::State::About] = ui_mapping;

    return mappings;
}

inline menu::State get_initial_input_layer() { return menu::State::Root; }

}  // namespace game
