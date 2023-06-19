#pragma once

#include "../engine.h"
#include "../engine/app.h"
#include "../external_include.h"

struct MenuLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;

    MenuLayer() : Layer("Menu") { ui_context.reset(new ui::UIContext()); }

    virtual ~MenuLayer() {}

    virtual void onEvent(Event& event) override {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(
            std::bind(&MenuLayer::onKeyPressed, this, std::placeholders::_1));
        dispatcher.dispatch<GamepadButtonPressedEvent>(std::bind(
            &MenuLayer::onGamepadButtonPressed, this, std::placeholders::_1));
        dispatcher.dispatch<GamepadAxisMovedEvent>(std::bind(
            &MenuLayer::onGamepadAxisMoved, this, std::placeholders::_1));
    }

    bool onGamepadAxisMoved(GamepadAxisMovedEvent& event) {
        if (MenuState::get().is_not(menu::State::Root)) return false;
        return ui_context.get()->process_gamepad_axis_event(event);
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        if (MenuState::get().is_not(menu::State::Root)) return false;
        return ui_context.get()->process_keyevent(event);
    }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) {
        if (MenuState::get().is_not(menu::State::Root)) return false;
        return ui_context.get()->process_gamepad_button_event(event);
    }

    void play_music() {
        // TODO better music playing wrapper so we can not duplicate this
        // everwhere
        // TODO music stops playing when you grab title bar
        auto m = MusicLibrary::get().get("theme");
        if (!raylib::IsMusicStreamPlaying(m)) {
            raylib::PlayMusicStream(m);
        }
        raylib::UpdateMusicStream(m);
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
                       "Play")) {
                MenuState::get().set(menu::State::Network);
            }
            padding(*ui::components::mk_but_pad());
            if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                       "About")) {
                MenuState::get().set(menu::State::About);
            }
            padding(*ui::components::mk_but_pad());
            if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                       "Settings")) {
                MenuState::get().set(menu::State::Settings);
            }
            padding(*ui::components::mk_but_pad());
            if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                       "Exit")) {
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
            // TODO this somewhere should match the one in app.h
            text(*title_text, "Pharmasea");
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

    virtual void onDraw(float dt) override {
        ZoneScoped;
        if (MenuState::get().is_not(menu::State::Root)) return;
        PROFILE();
        ext::clear_background(ui_context->active_theme().background);

        ui_context->begin(dt);

        auto root = ui::components::mk_root();

        ui_context->push_parent(root);
        {
            draw_menu_buttons();
            draw_title_section();
        }
        ui_context->pop_parent();
        ui_context->end(root.get());
    }
};