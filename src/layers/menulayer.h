#pragma once

#include "../engine.h"
#include "../engine/app.h"
#include "../engine/util.h"
#include "../external_include.h"
#include "../local_ui.h"

struct MenuLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;

    MenuLayer()
        : Layer(strings::menu::MENU),
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

        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};

        auto [left, right] = rect::vsplit(window, 50);

        left = rect::rpad(rect::lpad(left, 20), 90);

        // TODO add non rounded rectangle
        div(left, color::brownish_purple);

        left = rect::rpad(rect::lpad(left, 10), 90);

        auto [title, buttons] = rect::hsplit(left, 50);

        // Title
        {
            title = rect::tpad(rect::bpad(title, 50), 30);
            text(Widget{title}, TranslatableString(strings::GAME_NAME));
        }

        // Buttons
        {
            buttons = rect::tpad(rect::all_pad(buttons, 10), 10);
            // div(buttons, color::coffee);

            auto [buttons_top, buttons_bottom] = rect::hsplit<2>(buttons);
            auto [rect1, rect2] = rect::vsplit<2>(buttons_top);
            auto [rect3, rect4] = rect::vsplit<2>(buttons_bottom);

            rect1 = rect::rpad(rect::vpad(rect1, 20), 95);
            rect2 = rect::lpad(rect::vpad(rect2, 20), 5);

            rect3 = rect::rpad(rect::vpad(rect3, 20), 95);
            rect4 = rect::lpad(rect::vpad(rect4, 20), 5);

            if (ps::button(Widget{rect1},
                           TranslatableString(strings::i18n::PLAY))) {
                MenuState::get().set(menu::State::Network);
            }
            if (ps::button(Widget{rect2},
                           TranslatableString(strings::i18n::ABOUT))) {
                MenuState::get().set(menu::State::About);
            }
            if (ps::button(Widget{rect3},
                           TranslatableString(strings::i18n::SETTINGS))) {
                MenuState::get().set(menu::State::Settings);
            }
            if (ps::button(Widget{rect4},
                           TranslatableString(strings::i18n::EXIT))) {
                App::get().close();
            }
        }

        // Ext Buttons
        {
            auto [_, footer] = rect::hsplit(right, 80);
            auto ext_buttons = rect::rpad(rect::lpad(footer, 70), 80);
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
