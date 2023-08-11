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
        root_box =
            load_ui("resources/html/menu.html", {0, 0, WIN_WF(), WIN_HF()});
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

    void draw_menu_buttons() {
        using namespace ui;
        padding(*ui::components::mk_padding(Size_Px(100.f, 1.f),
                                            Size_Px(WIN_HF(), 1.f)));

        auto content =
            ui_context->own(Widget({.mode = Children, .strictness = 1.f},
                                   Size_Pct(1.f, 1.0f), Column));
        div(*content);

        ui_context->push_parent(content);
        {
            padding(*ui::components::mk_padding(Size_Px(100.f, 1.f),
                                                Size_Pct(1.f, 0.f)));
            if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                       text_lookup(strings::i18n::PLAY))) {
                MenuState::get().set(menu::State::Network);
            }
            padding(*ui::components::mk_but_pad());
            if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                       text_lookup(strings::i18n::ABOUT))) {
                MenuState::get().set(menu::State::About);
            }
            padding(*ui::components::mk_but_pad());
            if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                       text_lookup(strings::i18n::SETTINGS))) {
                MenuState::get().set(menu::State::Settings);
            }
            padding(*ui::components::mk_but_pad());
            if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                       text_lookup(strings::i18n::EXIT))) {
                App::get().close();
            }

            padding(*ui::components::mk_padding(Size_Px(100.f, 1.f),
                                                Size_Pct(1.f, 0.f)));
        }
        ui_context->pop_parent();
    }

    void draw_title_section() {
        using namespace ui;

        padding(*ui::components::mk_padding(Size_Px(200.f, 1.f),
                                            Size_Px(WIN_HF(), 1.f)));

        auto title_card = ui_context->own(Widget(
            Size_Pct(1.f, 0.f), Size_Px(WIN_HF(), 1.f), GrowFlags::Column));
        div(*title_card);
        ui_context->push_parent(title_card);
        {
            padding(*ui::components::mk_padding(Size_Pct(1.f, 0.f),
                                                Size_Px(100.f, 1.f)));
            auto title_text = ui_context->own(
                Widget(Size_Pct(1.f, 0.5f), Size_Px(100.f, 1.f)));
            text(*title_text, strings::GAME_NAME);
        }
        ui_context->pop_parent();
    }

    void draw_external_icons() {
        using namespace ui;

        padding(*ui::components::mk_padding(Size_Px(500.f, 1.f),
                                            Size_Px(WIN_HF(), 1.f)));

        auto third_col = ui_context->own(Widget(
            Size_Pct(1.f, 0.f), Size_Px(WIN_HF(), 1.f), GrowFlags::Column));
        div(*third_col);
        ui_context->push_parent(third_col);
        {
            padding(*ui::components::mk_padding(
                Size_Pct(1.f, 0.f), Size_Px(WIN_HF() - 150.f, 1.f)));

            auto button = ui::components::mk_button(MK_UUID(id, ROOT_ID),
                                                    Size_Pct(0.05f, 1.f),
                                                    Size_Pct(0.075f, 1.f));

            if (image_button(*button, "discord")) {
                util::open_url(strings::urls::DISCORD);
            }
        }
        ui_context->pop_parent();
    }

    virtual void onUpdate(float) override {
        ZoneScoped;
        if (MenuState::get().is_in_menu()) play_music();
        if (MenuState::get().is_not(menu::State::Root)) return;
        PROFILE();
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

    virtual void onDraw(float) override {
        ZoneScoped;
        if (MenuState::get().is_not(menu::State::Root)) return;
        PROFILE();
        ext::clear_background(ui_context->active_theme().background);

        elements::begin();

        render_ui(ui_context, root_box, {0, 0, WIN_WF(), WIN_HF()},
                  std::bind(&MenuLayer::process_on_click, *this,
                            std::placeholders::_1));

        elements::end(ui_context);

        // ui_context->begin(dt);
        //
        // auto root = ui::components::mk_root();
        //
        // ui_context->push_parent(root);
        // {
        // draw_menu_buttons();
        // draw_title_section();
        // draw_external_icons();
        // }
        // ui_context->pop_parent();
        // ui_context->end(root.get());
    }
};
