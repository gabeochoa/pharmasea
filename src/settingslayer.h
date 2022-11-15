#pragma once

#include "event.h"
#include "external_include.h"
//
#include "layer.h"
#include "menu.h"
#include "settings.h"
#include "ui.h"
#include "ui_widget.h"
#include "uuid.h"

using namespace ui;

struct SettingsLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;
    bool windowSizeDropdownState = false;
    int windowSizeDropdownIndex = 0;

    SettingsLayer() : Layer("Settings") {
        ui_context.reset(new ui::UIContext());
    }
    virtual ~SettingsLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    virtual void onEvent(Event& event) override {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(std::bind(
            &SettingsLayer::onKeyPressed, this, std::placeholders::_1));
        dispatcher.dispatch<GamepadButtonPressedEvent>(
            std::bind(&SettingsLayer::onGamepadButtonPressed, this,
                      std::placeholders::_1));
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        if (Menu::get().state != Menu::State::Settings) return false;
        if (event.keycode == KEY_ESCAPE) {
            Menu::get().state = Menu::State::Root;
            return true;
        }
        return ui_context.get()->process_keyevent(event);
    }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) {
        if (Menu::get().state != Menu::State::Settings) return false;
        return ui_context.get()->process_gamepad_button_event(event);
    }

    void draw_ui(float dt) {
        using namespace ui;
        // TODO select the acurate options based on current settings
        // auto& settings = Settings::get();

        // TODO move to input
        bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        vec2 mousepos = GetMousePosition();

        ui_context->begin(mouseDown, mousepos, dt);

        auto root = ui::components::mk_root();

        //
        Widget volume_slider_container({.mode = Children}, {.mode = Children},
                                       GrowFlags::Row);

        Widget volume_widget(
            // TODO replace with text size
            {.mode = Pixels, .value = 100.f, .strictness = 1.f},
            {.mode = Pixels, .value = 50.f, .strictness = 1.f});

        Widget slider_widget(
            MK_UUID(id, ROOT_ID),
            {.mode = Pixels, .value = 100.f, .strictness = 1.f},
            {.mode = Pixels, .value = 30.f, .strictness = 1.f});

        Widget back_button(MK_UUID(id, ROOT_ID),
                           {.mode = Pixels, .value = 120.f},
                           {.mode = Pixels, .value = 50.f});

        Widget window_size_container({.mode = Children}, {.mode = Children},
                                     GrowFlags::Row);
        //

        ui_context->push_parent(root);
        {
            auto left_padding = ui_context->own(
                Widget({.mode = Pixels, .value = 100.f, .strictness = 1.f},
                       {.mode = Pixels, .value = WIN_H, .strictness = 1.f}));

            auto content = ui_context->own(Widget(
                {.mode = Children, .strictness = 1.f},
                {.mode = Percent, .value = 1.f, .strictness = 1.0f}, Column));
            padding(*left_padding);
            div(*content);
            ui_context->push_parent(content);
            {
                auto top_padding = ui_context->own(
                    Widget({.mode = Pixels, .value = 100.f, .strictness = 1.f},
                           {.mode = Percent, .value = 1.f, .strictness = 0.f}));
                padding(*top_padding);
                div(volume_slider_container);
                ui_context->push_parent(&volume_slider_container);
                {
                    text(volume_widget, "Master Volume");

                    auto left_padding2 = ui_context->own(Widget(
                        {.mode = Pixels, .value = 100.f, .strictness = 1.f},
                        {.mode = Pixels, .value = 100.f, .strictness = 1.f}));
                    padding(*left_padding2);

                    float* mv = &(Settings::get().data.master_volume);
                    if (slider(slider_widget, false, mv, 0.f, 1.f)) {
                        Settings::get().update_master_volume(*mv);
                    }
                }
                ui_context->pop_parent();

                text(*ui::components::mk_text(), "Show Streamer Safe Box");
                auto checkbox_widget = ui_context->own(
                    Widget(MK_UUID(id, ROOT_ID), Size_Px(75.f, 0.5f),
                           Size_Px(25.f, 1.f)));
                bool sssb = Settings::get().data.show_streamer_safe_box;
                if (checkbox(*checkbox_widget, &sssb)) {
                    Settings::get().update_streamer_safe_box(sssb);
                    std::cout << "checkbox changed" << std::endl;
                }

                if (button(back_button, "Back")) {
                    Menu::get().state = Menu::State::Root;
                }
                padding(*ui_context->own(Widget(
                    {.mode = Pixels, .value = 100.f, .strictness = 1.f},
                    {.mode = Percent, .value = 1.f, .strictness = 0.f})));
            }
            ui_context->pop_parent();
        }
        ui_context->pop_parent();

        ui_context->end(root.get());
    }

    virtual void onUpdate(float) override {
        if (Menu::get().state != Menu::State::Settings) return;
        SetExitKey(KEY_NULL);

        // TODO with gamelayer, support events
        if (minimized) {
            return;
        }
    }

    virtual void onDraw(float dt) override {
        if (Menu::get().state != Menu::State::Settings) return;
        // TODO with gamelayer, support events
        if (minimized) {
            return;
        }

        ClearBackground(ui_context->active_theme().background);
        draw_ui(dt);
    }
};
