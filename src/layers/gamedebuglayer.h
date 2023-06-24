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

struct GameDebugLayer : public Layer {
    bool debug_ui_enabled = false;
    bool network_ui_enabled = false;
    std::shared_ptr<ui::UIContext> ui_context;

    GameDebugLayer() : Layer("Game") {
        GLOBALS.set("debug_ui_enabled", &debug_ui_enabled);
        GLOBALS.set("network_ui_enabled", &network_ui_enabled);
        ui_context.reset(new ui::UIContext());
    }
    virtual ~GameDebugLayer() {}

    virtual void onEvent(Event& event) override {
        if (!MenuState::s_in_game()) return;
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(std::bind(
            &GameDebugLayer::onKeyPressed, this, std::placeholders::_1));
        dispatcher.dispatch<GamepadButtonPressedEvent>(
            std::bind(&GameDebugLayer::onGamepadButtonPressed, this,
                      std::placeholders::_1));
        dispatcher.dispatch<GamepadAxisMovedEvent>(std::bind(
            &GameDebugLayer::onGamepadAxisMoved, this, std::placeholders::_1));
    }

    bool onGamepadAxisMoved(GamepadAxisMovedEvent&) { return false; }

    void toggle_to_planning() {
        GameState::s_toggle_to_planning();
        EntityHelper::invalidatePathCache();
    }

    void toggle_to_inround() {
        GameState::s_toggle_to_inround();
        EntityHelper::invalidatePathCache();
    }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) {
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

    bool onKeyPressed(KeyPressedEvent& event) {
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

    virtual void onDraw(float dt) override {
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

        debug_ui(dt);
    }

   private:
    // TODO implement or remove
    void draw_all_debug_ui() {}

    void debug_ui(float dt) {
        if (!debug_ui_enabled) return;
        using namespace ui;

        ui_context->begin(dt);

        auto root = ui_context->own(Widget(
            Size_Px(WIN_WF(), 1.f), Size_Px(WIN_HF(), 1.f), GrowFlags::Row));
        ui_context->push_parent(root);
        {
            auto left_padding = ui_context->own(
                Widget(Size_Px(100.f, 1.f), Size_Px(WIN_HF(), 1.f)));

            auto content =
                ui_context->own(Widget({.mode = Children, .strictness = 1.f},
                                       Size_Pct(1.f, 1.0f), Column));

            padding(*left_padding);
            div(*content);
            ui_context->push_parent(content);
            {
                auto top_padding = ui_context->own(
                    Widget(Size_Px(100.f, 1.f), Size_Pct(1.f, 0.f)));
                padding(*top_padding);
                {  //
                    draw_all_debug_ui();
                }
                padding(*ui_context->own(
                    Widget(Size_Px(100.f, 1.f), Size_Pct(1.f, 0.f))));
            }
            ui_context->pop_parent();
        }
        ui_context->pop_parent();
        ui_context->end(root.get());
    }
};
