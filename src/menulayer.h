#pragma once

#include "external_include.h"
#include "layer.h"
#include "menu.h"
#include "network/network.h"
#include "profile.h"
#include "raylib.h"
#include "settings.h"
#include "ui.h"
#include "ui_theme.h"
#include "ui_widget.h"
#include "uuid.h"

struct MenuLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;

    MenuLayer() : Layer("Menu") {
        minimized = false;

        ui_context.reset(new ui::UIContext());
        ui_context->init();
        ui_context->set_font(Preload::get().font);
        // TODO we should probably enforce that you cant do this
        // and we should have ->set_base_theme()
        // and push_theme separately, if you end() with any stack not empty...
        // thats a flag
        ui_context->push_theme(ui::DEFAULT_THEME);
    }
    virtual ~MenuLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

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
        if (Menu::get().is_not(Menu::State::Root)) return false;
        return ui_context.get()->process_gamepad_axis_event(event);
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        if (Menu::get().is_not(Menu::State::Root)) return false;
        return ui_context.get()->process_keyevent(event);
    }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) {
        if (Menu::get().is_not(Menu::State::Root)) return false;
        return ui_context.get()->process_gamepad_button_event(event);
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
                Menu::get().set(Menu::State::Network);
            }
            padding(*ui::components::mk_but_pad());
            if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                       "About")) {
                Menu::get().set(Menu::State::About);
            }
            padding(*ui::components::mk_but_pad());
            if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                       "Settings")) {
                Menu::get().set(Menu::State::Settings);
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
            text(*title_text, "Pharmasea");
        }
        ui_context->pop_parent();
    }

    void draw_ui(float dt) {
        using namespace ui;

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

    virtual void onUpdate(float) override {
        if (Menu::get().is_not(Menu::State::Root)) return;
        PROFILE();
        SetExitKey(KEY_ESCAPE);
    }

    virtual void onDraw(float dt) override {
        if (Menu::get().is_not(Menu::State::Root)) return;
        PROFILE();
        ClearBackground(ui_context->active_theme().background);
        draw_ui(dt);
    }
};
