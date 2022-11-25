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

    virtual void onEvent(Event& event) override {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(std::bind(
            &SettingsLayer::onKeyPressed, this, std::placeholders::_1));
        dispatcher.dispatch<GamepadButtonPressedEvent>(
            std::bind(&SettingsLayer::onGamepadButtonPressed, this,
                      std::placeholders::_1));
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        if (Menu::get().is_not(Menu::State::Settings)) return false;
        if (event.keycode == KEY_ESCAPE) {
            Menu::get().go_back();
            return true;
        }
        return ui_context.get()->process_keyevent(event);
    }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) {
        if (Menu::get().is_not(Menu::State::Settings)) return false;
        return ui_context.get()->process_gamepad_button_event(event);
    }

    void streamer_safe_box() {
        auto container = ui_context->own(
            Widget({.mode = Children}, {.mode = Children}, GrowFlags::Row));

        div(*container);
        ui_context->push_parent(container);
        {
            text(*ui::components::mk_text(), "Show Streamer Safe Box");
            auto checkbox_widget = ui_context->own(Widget(
                MK_UUID(id, ROOT_ID), Size_Px(75.f, 0.5f), Size_Px(25.f, 1.f)));
            bool sssb = Settings::get().data.show_streamer_safe_box;
            if (checkbox(*checkbox_widget, &sssb)) {
                Settings::get().update_streamer_safe_box(sssb);
            }
        }
        ui_context->pop_parent();
    }

    void master_volume() {
        auto volume_slider_container = ui_context->own(
            Widget({.mode = Children}, {.mode = Children}, GrowFlags::Row));

        div(*volume_slider_container);
        ui_context->push_parent(volume_slider_container);
        {
            text(*ui::components::mk_text(), "Master Volume");
            padding(*ui::components::mk_padding(Size_Px(100.f, 1.f),
                                                Size_Px(100.f, 1.f)));

            auto slider_widget = ui_context->own(Widget(
                MK_UUID(id, ROOT_ID), Size_Px(100.f, 1.f), Size_Px(30.f, 1.f)));

            float* mv = &(Settings::get().data.master_volume);
            if (slider(*slider_widget, false, mv, 0.f, 1.f)) {
                Settings::get().update_master_volume(*mv);
            }
        }
        ui_context->pop_parent();
    }

    void music_volume() {
        auto volume_slider_container = ui_context->own(
            Widget({.mode = Children}, {.mode = Children}, GrowFlags::Row));

        div(*volume_slider_container);
        ui_context->push_parent(volume_slider_container);
        {
            text(*ui::components::mk_text(), "Music Volume");
            padding(*ui::components::mk_padding(Size_Px(100.f, 1.f),
                                                Size_Px(100.f, 1.f)));

            auto slider_widget = ui_context->own(Widget(
                MK_UUID(id, ROOT_ID), Size_Px(100.f, 1.f), Size_Px(30.f, 1.f)));

            float* mv = &(Settings::get().data.music_volume);
            if (slider(*slider_widget, false, mv, 0.f, 1.f)) {
                Settings::get().update_music_volume(*mv);
            }
        }
        ui_context->pop_parent();
    }

    void resolution_switcher() {
        auto resolution_switcher_container = ui_context->own(
            Widget({.mode = Children}, {.mode = Children}, GrowFlags::Row));

        div(*resolution_switcher_container);
        ui_context->push_parent(resolution_switcher_container);
        {
            text(*ui::components::mk_text(), "Resolution");
            padding(*ui::components::mk_padding(Size_Px(100.f, 1.f),
                                                Size_Px(100.f, 1.f)));
            text(*ui::components::mk_text(), "Coming Soon");
        }
        ui_context->pop_parent();
    }

    void back_button() {
        if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID),
                                              Size_Px(120.f, 1.f),
                                              Size_Px(50.f, 1.f)),
                   "Back")) {
            Menu::get().go_back();
        }
        padding(*ui::components::mk_padding(Size_Px(100.f, 1.f),
                                            Size_Pct(0.5f, 0.f)));
    }

    void draw_ui(float dt) {
        using namespace ui;
        // TODO move to input
        bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        vec2 mousepos = GetMousePosition();

        ui_context->begin(mouseDown, mousepos, dt);

        auto root = ui::components::mk_root();

        ui_context->push_parent(root);
        {
            padding(*ui::components::mk_padding(Size_Px(100.f, 1.f),
                                                Size_Px(WIN_H, 1.f)));
            auto content =
                ui_context->own(Widget({.mode = Children, .strictness = 1.f},
                                       Size_Pct(1.f, 1.f), Column));
            div(*content);
            ui_context->push_parent(content);
            {
                padding(*ui::components::mk_padding(Size_Px(100.f, 1.f),
                                                    Size_Px(100.f, 0.5f)));
                master_volume();
                music_volume();
                resolution_switcher();
                streamer_safe_box();
                back_button();
            }
            ui_context->pop_parent();
        }
        ui_context->pop_parent();

        ui_context->end(root.get());
    }

    virtual void onUpdate(float) override {
        if (Menu::get().is_not(Menu::State::Settings)) return;
        SetExitKey(KEY_NULL);
    }

    virtual void onDraw(float dt) override {
        if (Menu::get().is_not(Menu::State::Settings)) return;
        ClearBackground(ui_context->active_theme().background);
        draw_ui(dt);
    }
};
