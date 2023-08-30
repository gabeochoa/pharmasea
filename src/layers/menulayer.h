#pragma once

#include "../engine.h"
#include "../engine/app.h"
#include "../engine/ui/ui.h"
#include "../engine/util.h"
#include "../external_include.h"

struct MenuLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;

    MenuLayer()
        : Layer(strings::menu::MENU),
          ui_context(std::make_shared<ui::UIContext>()) {}

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

    virtual void onDraw(float dt) override {
        if (MenuState::get().is_not(menu::State::Root)) return;
        ext::clear_background(ui::UI_THEME.background);
        using namespace ui;

        begin(ui_context, dt);

        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        auto [top, rest] = rect::hsplit(window, 33);
        auto [body, footer] = rect::hsplit(rest, 66);

        // Title
        {
            auto text_loc = rect::lpad(top, 15);
            text(Widget{text_loc}, text_lookup(strings::GAME_NAME));
        }

        // Buttons
        {
            auto [rect1, rect2, rect3, rect4] =
                rect::hsplit<4>(rect::hpad(body, 15), 20);

            if (button(Widget{rect1}, text_lookup(strings::i18n::PLAY))) {
                MenuState::get().set(menu::State::Network);
            }
            if (button(Widget{rect2}, text_lookup(strings::i18n::ABOUT))) {
                MenuState::get().set(menu::State::About);
            }
            if (button(Widget{rect3}, text_lookup(strings::i18n::SETTINGS))) {
                MenuState::get().set(menu::State::Settings);
            }
            if (button(Widget{rect4}, text_lookup(strings::i18n::EXIT))) {
                App::get().close();
            }
        }

        // Ext Buttons
        {
            auto ext_buttons = rect::rpad(rect::lpad(footer, 80), 90);
            auto [b1, b2] = rect::vsplit<2>(ext_buttons, 20);

            if (image_button(Widget{b1}, "discord")) {
                util::open_url(strings::urls::DISCORD);
            }
            // TODO chose the right color based on the theme
            if (image_button(Widget{b2}, "itch-white")) {
                util::open_url(strings::urls::ITCH);
            }
        }

        end();
    }
};
