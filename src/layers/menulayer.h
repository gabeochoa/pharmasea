#pragma once

#include "../engine.h"
#include "../engine/app.h"
#include "../engine/ui/ui.h"
#include "../engine/util.h"
#include "../external_include.h"

struct MenuLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;
    LayoutBox root_box;

    MenuLayer()
        : Layer(strings::menu::MENU),
          ui_context(std::make_shared<ui::UIContext>()) {
        root_box = load_ui("resources/html/menu.html", WIN_R());
    }

    virtual ~MenuLayer() {}

    bool onGamepadAxisMoved(GamepadAxisMovedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Root)) return false;
        return ui_context.get()->process_gamepad_axis_event(event);
    }

    bool onKeyPressed(KeyPressedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Root)) return false;
        return ui_context.get()->process_keyevent(event);
    }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Root)) return false;
        return ui_context.get()->process_gamepad_button_event(event);
    }

    void play_music() {
        if (ENABLE_SOUND) {
            // TODO better music playing wrapper so we can not duplicate this
            // everwhere
            // TODO music stops playing when you grab title bar
            auto m = MusicLibrary::get().get(strings::music::THEME);
            if (!raylib::IsMusicStreamPlaying(m)) {
                raylib::PlayMusicStream(m);
            }
            raylib::UpdateMusicStream(m);
        }
    }

    virtual void onUpdate(float) override {
        if (MenuState::get().is_in_menu()) play_music();
        if (MenuState::get().is_not(menu::State::Root)) return;
        raylib::SetExitKey(raylib::KEY_ESCAPE);
    }

    void process_on_click(const std::string& id) {
        switch (hashString(id)) {
            case hashString(strings::i18n::PLAY):
                MenuState::get().set(menu::State::Network);
                break;

            case hashString(strings::i18n::ABOUT):
                MenuState::get().set(menu::State::About);
                break;

            case hashString(strings::i18n::SETTINGS):
                MenuState::get().set(menu::State::Settings);
                break;

            case hashString(strings::i18n::EXIT):
                App::get().close();
                break;
        }
        // TODO return true when this was correctly caught
        // return true;
    }

    virtual void onDraw(float dt) override {
        if (MenuState::get().is_not(menu::State::Root)) return;
        ext::clear_background(ui_context->active_theme().background);

        elements::begin(ui_context, dt);

        render_ui(root_box, {0, 0, WIN_WF(), WIN_HF()},
                  std::bind(&MenuLayer::process_on_click, *this,
                            std::placeholders::_1));

        elements::end();
    }
};
