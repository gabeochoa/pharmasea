#pragma once

#include "../engine.h"
#include "../engine/app.h"
#include "../engine/svg_renderer.h"
#include "../engine/util.h"
#include "../external_include.h"
#include "../local_ui.h"

struct MenuLayer : public Layer {
    SVGRenderer svg;
    std::shared_ptr<ui::UIContext> ui_context;

    MenuLayer()
        : Layer(strings::menu::MENU),
          svg(SVGRenderer("main_menu")),
          ui_context(std::make_shared<ui::UIContext>()) {}

    virtual ~MenuLayer() {}

    bool onGamepadAxisMoved(GamepadAxisMovedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Root)) return false;
        return ui_context->process_gamepad_axis_event(event);
    }

    bool onKeyPressed(KeyPressedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Root)) return false;
        return ui_context->process_keyevent(event);
    }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Root)) return false;
        return ui_context->process_gamepad_button_event(event);
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

    virtual void onDraw(float dt) override {
        if (MenuState::get().is_not(menu::State::Root)) return;
        ext::clear_background(ui::UI_THEME.background);
        using namespace ui;

        begin(ui_context, dt);

        svg.draw_background();

        svg.text("Title", TranslatableString(strings::GAME_NAME));

        if (svg.button("PlayButton", TranslatableString(strings::i18n::Play))) {
            MenuState::get().set(menu::State::Network);
        }

        if (svg.button("SettingsButton",
                       TranslatableString(strings::i18n::Settings))) {
            MenuState::get().set(menu::State::Settings);
        }

        if (svg.button("AboutButton",
                       TranslatableString(strings::i18n::About))) {
            MenuState::get().set(menu::State::About);
        }

        if (svg.button("ExitButton", TranslatableString(strings::i18n::Exit))) {
            App::get().close();
        }

        if (svg.button("discord", NO_TRANSLATE(""))) {
            util::open_url(strings::urls::DISCORD);
        }

        if (svg.button("itch-white", NO_TRANSLATE(""))) {
            util::open_url(strings::urls::ITCH);
        }

        end();
    }
};
