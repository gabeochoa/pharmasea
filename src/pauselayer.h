
#pragma once

#include "engine.h"
#include "engine/globals_register.h"
#include "engine/layer.h"
#include "external_include.h"
#include "statemanager.h"

using namespace ui;

struct BasePauseLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;
    game::State back_state;
    game::State enabled_state;

    BasePauseLayer(const char* name, game::State b_state, game::State e_state)
        : Layer(name), back_state(b_state), enabled_state(e_state) {
        ui_context.reset(new ui::UIContext());
    }
    virtual ~BasePauseLayer() {}

    virtual void onEvent(Event& event) override {
        if (GameState::get().is_not(enabled_state)) return;
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(std::bind(
            &BasePauseLayer::onKeyPressed, this, std::placeholders::_1));
        dispatcher.dispatch<GamepadButtonPressedEvent>(
            std::bind(&BasePauseLayer::onGamepadButtonPressed, this,
                      std::placeholders::_1));
    }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) {
        if (KeyMap::get_button(menu::State::Game, InputName::Pause) ==
            event.button) {
            GameState::get().go_back();
            return true;
        }
        return ui_context.get()->process_gamepad_button_event(event);
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        if (KeyMap::get_key_code(menu::State::Game, InputName::Pause) ==
            event.keycode) {
            GameState::get().go_back();
            return true;
        }
        return ui_context.get()->process_keyevent(event);
    }

    virtual void onUpdate(float) override {}

    virtual void onDraw(float dt) override {
        // Note: theres no pausing outside the game, so dont render
        if (!MenuState::s_in_game()) return;

        if (GameState::get().is_not(enabled_state)) return;

        // NOTE: We specifically dont clear background
        // because people are used to pause menu being an overlay

        ui_context->begin(dt);

        auto root = ui_context->own(Widget(
            Size_Px(WIN_WF(), 1.f), Size_Px(WIN_HF(), 1.f), GrowFlags::Row));

        ui_context->push_parent(root);
        {
            auto left_padding = ui_context->own(
                Widget(Size_Px(100.f, 1.f), Size_Px(WIN_HF(), 1.f)));

            auto content =
                ui_context->own(Widget({.mode = Children, .strictness = 1.f},
                                       Size_Pct(1.f, 1.f), Column));

            padding(*left_padding);
            div(*content);
            ui_context->push_parent(content);
            {
                auto top_padding = ui_context->own(
                    Widget(Size_Px(100.f, 1.f), Size_Pct(1.f, 0.f)));
                padding(*top_padding);
                {
                    if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                               "Continue")) {
                        GameState::get().go_back();
                    }
                    if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                               "Settings")) {
                        MenuState::get().set(menu::State::Settings);
                    }
                    if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                               "Quit")) {
                        MenuState::get().reset();
                        GameState::get().reset();
                    }
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

struct PauseLayer : public BasePauseLayer {
    PauseLayer()
        : BasePauseLayer("Pause", game::State::InRound, game::State::Paused) {}
    virtual ~PauseLayer() {}
};
