
#pragma once

#include "../engine.h"
#include "../engine/ui/ui.h"
#include "../external_include.h"

using namespace ui;

struct BasePauseLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;
    game::State enabled_state;
    LayoutBox pause_ui;

    BasePauseLayer(const char* name, game::State e_state)
        : Layer(name), enabled_state(e_state) {
        ui_context = std::make_shared<ui::UIContext>();
        pause_ui = load_ui("resources/html/pause.html", WIN_R());
    }
    virtual ~BasePauseLayer() {}

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) override {
        if (GameState::get().is_not(enabled_state)) return false;
        if (KeyMap::get_button(menu::State::Game, InputName::Pause) ==
            event.button) {
            GameState::get().go_back();
            return true;
        }
        return ui_context.get()->process_gamepad_button_event(event);
    }

    bool onKeyPressed(KeyPressedEvent& event) override {
        if (GameState::get().is_not(enabled_state)) return false;
        if (KeyMap::get_key_code(menu::State::Game, InputName::Pause) ==
            event.keycode) {
            GameState::get().go_back();
            return true;
        }
        return ui_context.get()->process_keyevent(event);
    }

    virtual void onUpdate(float) override {}

    void process_on_click(const std::string& id) {
        switch (hashString(id)) {
            case hashString(strings::i18n::CONTINUE):
                GameState::get().go_back();
                break;
            case hashString(strings::i18n::SETTINGS):
                MenuState::get().set(menu::State::Settings);
                break;
            case hashString("Reload Config"):
                Preload::get().reload_config();
                break;
            case hashString(strings::i18n::QUIT):
                MenuState::get().reset();
                GameState::get().reset();
                break;
        }
        // TODO return true when this was correctly caught
        // return true;
    }

    virtual void onDraw(float dt) override {
        // Note: theres no pausing outside the game, so dont render
        if (!MenuState::s_in_game()) return;

        if (GameState::get().is_not(enabled_state)) return;

        // NOTE: We specifically dont clear background
        // because people are used to pause menu being an overlay

        elements::begin(ui_context, dt);

        render_ui(pause_ui, WIN_R(),
                  std::bind(&BasePauseLayer::process_on_click, *this,
                            std::placeholders::_1));

        elements::end();
    }
};

struct PauseLayer : public BasePauseLayer {
    PauseLayer() : BasePauseLayer("Pause", game::State::Paused) {}
    virtual ~PauseLayer() {}
};
