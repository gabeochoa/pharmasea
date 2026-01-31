#pragma once

namespace game {

// All mappable game actions
// These map to afterhours::input::ProvidesLayeredInputMapping
enum GameAction {
    // Movement
    PlayerForward = 0,
    PlayerBack,
    PlayerLeft,
    PlayerRight,

    // Player Actions
    PlayerPickup,
    PlayerRotateFurniture,
    PlayerDoWork,
    PlayerHandTruckInteract,

    // UI Navigation
    Pause,
    MenuBack,
    WidgetNext,
    WidgetBack,
    WidgetPress,
    ValueUp,
    ValueDown,
    ValueLeft,
    ValueRight,

    // Game-specific
    ShowRecipeBook,
    RecipeNext,

    // Debug (only in debug builds)
    ToggleDebug,
    ToggleNetworkView,
    ToggleLobby,
    ToggleDebugSettings,
    SkipIngredientMatch,

    // Modifier keys (for UI text input)
    WidgetMod,
    WidgetBackspace,
    WidgetCtrl,
    WidgetPaste,
};

}  // namespace game
