#pragma once

#include "../drawing_util.h"
#include "../engine/event.h"
#include "../external_include.h"
//
#include "../engine.h"
#include "../globals.h"
#include "../preload.h"
//
#include "../camera.h"
#include "../engine/layer.h"
#include "../engine/statemanager.h"
#include "../entityhelper.h"
#include "raylib.h"

struct GameDebugLayer : public Layer {
    bool debug_ui_enabled = false;
    bool network_ui_enabled = false;
    std::shared_ptr<ui::UIContext> ui_context;

    GameDebugLayer() : Layer(strings::menu::GAME) {
        GLOBALS.set("debug_ui_enabled", &debug_ui_enabled);
        GLOBALS.set("network_ui_enabled", &network_ui_enabled);
        ui_context = std::make_shared<ui::UIContext>();
    }
    virtual ~GameDebugLayer() {}

    void toggle_to_planning() {
        GameState::get().toggle_to_planning();
        EntityHelper::invalidatePathCache();
    }

    void toggle_to_inround() {
        GameState::get().toggle_to_inround();
        EntityHelper::invalidatePathCache();
    }

    bool onGamepadAxisMoved(GamepadAxisMovedEvent&) override { return false; }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) override {
        if (!MenuState::s_in_game()) return false;
        if (KeyMap::get_button(menu::State::Game,
                               InputName::ToggleToPlanning) == event.button) {
            toggle_to_planning();
            return true;
        }
        if (KeyMap::get_button(menu::State::Game, InputName::ToggleToInRound) ==
            event.button) {
            toggle_to_inround();
            return true;
        }
        if (KeyMap::get_button(menu::State::Game, InputName::ToggleDebug) ==
            event.button) {
            debug_ui_enabled = !debug_ui_enabled;
            return true;
        }
        if (KeyMap::get_button(menu::State::Game,
                               InputName::ToggleNetworkView) == event.button) {
            network_ui_enabled = !network_ui_enabled;
            return true;
        }
        return ui_context.get()->process_gamepad_button_event(event);
    }

    bool onKeyPressed(KeyPressedEvent& event) override {
        if (!MenuState::s_in_game()) return false;
        if (KeyMap::get_key_code(menu::State::Game,
                                 InputName::ToggleToPlanning) ==
            event.keycode) {
            toggle_to_planning();
            return true;
        }
        if (KeyMap::get_key_code(menu::State::Game,
                                 InputName::ToggleToInRound) == event.keycode) {
            toggle_to_inround();
            return true;
        }
        if (KeyMap::get_key_code(menu::State::Game, InputName::ToggleLobby) ==
            event.keycode) {
            EntityHelper::invalidatePathCache();
            GameState::get().set(game::State::Lobby);
            return true;
        }
        if (KeyMap::get_key_code(menu::State::Game, InputName::ToggleDebug) ==
            event.keycode) {
            debug_ui_enabled = !debug_ui_enabled;
            if (debug_ui_enabled) {
                raylib::EnableCursor();
            } else {
                // raylib::DisableCursor();
            }

            return true;
        }
        if (KeyMap::get_key_code(menu::State::Game,
                                 InputName::ToggleNetworkView) ==
            event.keycode) {
            network_ui_enabled = !network_ui_enabled;
            return true;
        }
        return ui_context.get()->process_keyevent(event);
    }

    virtual void onUpdate(float) override {
        if (!MenuState::s_in_game()) return;
    }

    virtual void onDraw(float) override {
        if (!MenuState::s_in_game()) return;

        if (!debug_ui_enabled) {
            DrawTextEx(Preload::get().font, "Press \\ to toggle debug UI",
                       vec2{200, 70}, 20, 0, RED);
        }

        if (!network_ui_enabled) {
            DrawTextEx(
                Preload::get().font,
                "Press = to toggle network debug UI (only works if host)",
                vec2{200, 100}, 20, 0, RED);
        }
    }
};
