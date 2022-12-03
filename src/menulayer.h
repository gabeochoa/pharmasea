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

    void draw_ui(float dt) {
        using namespace ui;

        // TODO move to input
        bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        vec2 mousepos = GetMousePosition();

        const SizeExpectation padd_x = {
            .mode = Pixels, .value = 120.f, .strictness = 0.9f};
        const SizeExpectation padd_y = {
            .mode = Pixels, .value = 25.f, .strictness = 0.5f};

        ui_context->begin(mouseDown, mousepos, dt);

        ui::Widget root({.mode = Pixels, .value = WIN_WF(), .strictness = 1.f},
                        {.mode = Pixels, .value = WIN_HF(), .strictness = 1.f},
                        GrowFlags::Row);

        Widget left_padding(
            {.mode = Pixels, .value = 100.f, .strictness = 1.f},
            {.mode = Pixels, .value = WIN_HF(), .strictness = 1.f});

        Widget content({.mode = Children, .strictness = 1.f},
                       {.mode = Percent, .value = 1.f, .strictness = 1.0f},
                       Column);

        Widget top_padding({.mode = Pixels, .value = 100.f, .strictness = 1.f},
                           {.mode = Percent, .value = 1.f, .strictness = 0.f});

        const SizeExpectation button_x = {.mode = Pixels, .value = 130.f};
        const SizeExpectation button_y = {.mode = Pixels, .value = 50.f};

        Widget play_button(MK_UUID(id, ROOT_ID), button_x, button_y);
        Widget button_padding(padd_x, padd_y);
        Widget about_button(MK_UUID(id, ROOT_ID), button_x, button_y);
        Widget settings_button(MK_UUID(id, ROOT_ID), button_x, button_y);
        Widget exit_button(MK_UUID(id, ROOT_ID), button_x, button_y);
        Widget join_button(MK_UUID(id, ROOT_ID), button_x, button_y);

        Widget bottom_padding(
            {.mode = Pixels, .value = 100.f, .strictness = 1.f},
            {.mode = Percent, .value = 1.f, .strictness = 0.f});

        Widget title_left_padding(
            {.mode = Pixels, .value = 200.f, .strictness = 1.f},
            {.mode = Pixels, .value = WIN_HF(), .strictness = 1.f});

        ui::Widget title_card(
            {.mode = Percent, .value = 1.f, .strictness = 0.f},
            {.mode = Pixels, .value = WIN_HF(), .strictness = 1.f},
            GrowFlags::Column);

        Widget title_padding(
            {.mode = Percent, .value = 1.f, .strictness = 0.f},
            {.mode = Pixels, .value = 100.f, .strictness = 1.f});

        Widget title_text({.mode = Percent, .value = 1.f, .strictness = 0.5f},
                          {.mode = Pixels, .value = 100.f, .strictness = 1.f});

        ui_context->push_parent(&root);
        {
            padding(left_padding);
            div(content);

            ui_context->push_parent(&content);
            {
                padding(top_padding);
                if (button(join_button, "Play")) {
                    Menu::get().set(Menu::State::Network);
                }
                padding(button_padding);
                if (button(about_button, "About")) {
                    Menu::get().set(Menu::State::About);
                }
                padding(button_padding);
                if (button(settings_button, "Settings")) {
                    Menu::get().set(Menu::State::Settings);
                }
                padding(button_padding);
                if (button(exit_button, "Exit")) {
                    App::get().close();
                }

                auto sv = ui_context->own(Widget({
                    Size_Px(100.f, 1.f),
                    Size_Px(100.f, 1.f),
                }));
                scroll_view(
                    *sv,
                    []() {
                        auto container = ui::components::mk_column();
                        div(*container);
                        get().push_parent(container);
                        {
                            text(*ui::components::mk_text(), "Please");
                            padding(*ui::components::mk_but_pad());
                            text(*ui::components::mk_text(), "Stop");
                            padding(*ui::components::mk_but_pad());
                            text(*ui::components::mk_text(), "The");
                            padding(*ui::components::mk_but_pad());
                            text(*ui::components::mk_text(), "School");
                            padding(*ui::components::mk_but_pad());
                            text(*ui::components::mk_text(), "Bus");
                            padding(*ui::components::mk_but_pad());
                            DrawRectangleRounded({0, 0, 50, 50}, 0.15f, 4, RED);
                            DrawRectangleRounded({50, 50, 100, 100}, 0.15f, 4,
                                                 BLACK);
                            DrawRectangleRounded({100, 100, 150, 150}, 0.15f, 4,
                                                 BLUE);
                        }
                        get().pop_parent();
                    },
                    nullptr);

                padding(bottom_padding);
            }
            ui_context->pop_parent();

            padding(title_left_padding);

            div(title_card);
            ui_context->push_parent(&title_card);
            {
                padding(title_padding);
                text(title_text, "Pharmasea");
            }
            ui_context->pop_parent();
        }
        ui_context->pop_parent();
        ui_context->end(&root);
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
