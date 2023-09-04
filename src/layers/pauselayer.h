
#pragma once

#include "../engine.h"
#include "../engine/layer.h"
#include "../engine/ui/ui.h"
#include "../external_include.h"

using namespace ui;

struct BasePauseLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;
    game::State enabled_state;

    BasePauseLayer(const char* name, game::State e_state)
        : Layer(name), enabled_state(e_state) {
        ui_context = std::make_shared<ui::UIContext>();
    }
    virtual ~BasePauseLayer() {}

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) override {
        if (GameState::get().is_not(enabled_state)) return false;
        if (KeyMap::get_button(menu::State::Game, InputName::Pause) ==
            event.button) {
            GameState::get().go_back();
            return true;
        }
        return ui_context->process_gamepad_button_event(event);
    }

    bool onKeyPressed(KeyPressedEvent& event) override {
        if (GameState::get().is_not(enabled_state)) return false;
        if (KeyMap::get_key_code(menu::State::Game, InputName::Pause) ==
            event.keycode) {
            GameState::get().go_back();
            return true;
        }
        return ui_context->process_keyevent(event);
    }

    virtual void onUpdate(float) override {}

    virtual void onDraw(float dt) override {
        // Note: theres no pausing outside the game, so dont render
        if (!MenuState::s_in_game()) return;

        if (GameState::get().is_not(enabled_state)) return;

        // NOTE: We specifically dont clear background
        // because people are used to pause menu being an overlay

        using namespace ui;
        begin(ui_context, dt);

        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        auto body = rect::bpad(
            rect::tpad(rect::lpad(rect::rpad(window, 20), 20), 33), 66);

        auto [continue_button, settings, config, quit] =
            rect::hsplit<4>(body, 20);

        if (button(Widget{continue_button},
                   text_lookup(strings::i18n::CONTINUE))) {
            GameState::get().go_back();
        }
        if (button(Widget{settings}, text_lookup(strings::i18n::SETTINGS))) {
            MenuState::get().set(menu::State::Settings);
        }
        if (button(Widget{config}, "RELOAD CONFIGS")) {
            Preload::get().reload_config();
        }
        if (button(Widget{quit}, text_lookup(strings::i18n::QUIT))) {
            MenuState::get().reset();
            GameState::get().reset();
        }
        end();
    }
};

struct PauseLayer : public BasePauseLayer {
    PauseLayer() : BasePauseLayer("Pause", game::State::Paused) {}
    virtual ~PauseLayer() {}
};
