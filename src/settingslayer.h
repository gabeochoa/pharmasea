#pragma once

#include "external_include.h"
//
#include "input.h"
#include "layer.h"
#include "menu.h"
#include "settings.h"
#include "ui.h"
#include "ui_widget.h"
#include "uuid.h"

struct SettingsLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;
    bool windowSizeDropdownState = false;
    int windowSizeDropdownIndex = 0;

    float masterVolumeSliderValue = 0.5f;

    SettingsLayer() : Layer("Settings") {
        minimized = false;

        ui_context.reset(new ui::UIContext());
        ui_context.get()->init();
        ui_context->set_font(App::get().font);
        // TODO we should probably enforce that you cant do this
        // and we should have ->set_base_theme()
        // and push_theme separately, if you end() with any stack not empty...
        // thats a flag
        ui_context->push_theme(ui::DEFAULT_THEME);

        masterVolumeSliderValue = Settings::get().data.masterVolume;
    }
    virtual ~SettingsLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    virtual void onEvent(Event& event) override {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(std::bind(
            &SettingsLayer::onKeyPressed, this, std::placeholders::_1));
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        if (Menu::get().state != Menu::State::Settings) return false;
        if (event.keycode == KEY_ESCAPE) {
            Menu::get().state = Menu::State::Root;
            return true;
        }
        return ui_context.get()->process_keyevent(event);
    }

    void draw_ui() {
        using namespace ui;
        // TODO select the acurate options based on current settings
        auto& settings = Settings::get();

        // TODO move to input
        bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        vec2 mousepos = GetMousePosition();

        ui_context->begin(mouseDown, mousepos);

        ui::Widget root(
            {.mode = ui::SizeMode::Pixels, .value = WIN_W, .strictness = 1.f},
            {.mode = ui::SizeMode::Pixels, .value = WIN_H, .strictness = 1.f});
        root.growflags = ui::GrowFlags::Column;

        Widget volume_slider_container(
            {.mode = Children},
            {.mode = Percent, .value = 1.f, .strictness = 1.0f});
        volume_slider_container.growflags = GrowFlags::Row;
        Widget volume_widget(
            // TODO replace with text size
            {.mode = Pixels, .value = 100.f, .strictness = 1.f},
            {.mode = Pixels, .value = 50.f, .strictness = 1.f});

        Widget slider_widget(
            {.mode = Pixels, .value = 100.f, .strictness = 1.f},
            {.mode = Pixels, .value = 50.f, .strictness = 1.f});

        Widget back_button({.mode = Pixels, .value = 120.f},
                           {.mode = Pixels, .value = 50.f});

        Widget window_size_container(
            {.mode = Children},
            {.mode = Percent, .value = 1.f, .strictness = 1.0f});
        window_size_container.growflags = GrowFlags::Row;

        std::vector<std::string> dropdownConfigs;
        dropdownConfigs.push_back("960x540");
        dropdownConfigs.push_back("1920x1080");
        dropdownConfigs.push_back("800x600");

        Widget resolution_widget(
            // TODO replace with text size
            {.mode = Pixels, .value = 100.f, .strictness = 1.f},
            {.mode = Pixels, .value = 50.f, .strictness = 1.f});

        Widget dropdown_widget(
            {.mode = Pixels, .value = 100.f, .strictness = 1.f},
            {.mode = Pixels, .value = 50.f, .strictness = 1.f});

        ui_context->push_parent(&root);
        {
            // volume_sliders()
            div(volume_slider_container);
            ui_context->push_parent(&volume_slider_container);
            {
                text(volume_widget, "Master Volume");

                if (slider(slider_widget, false, &masterVolumeSliderValue, 0.f,
                           1.f)) {
                    Settings::get().update_master_volume(
                        masterVolumeSliderValue);
                }
            }
            ui_context->pop_parent();

            // window_size_dropdown()
            div(window_size_container);
            ui_context->push_parent(&window_size_container);
            {
                text(resolution_widget, "Resolution");
                if (dropdown(dropdown_widget, dropdownConfigs,
                             &windowSizeDropdownState,
                             &windowSizeDropdownIndex)) {
                    if (windowSizeDropdownIndex == 0) {
                        settings.update_window_size({960, 540});
                    } else if (windowSizeDropdownIndex == 1) {
                        settings.update_window_size({1920, 1080});
                    } else if (windowSizeDropdownIndex == 2) {
                        settings.update_window_size({800, 600});
                    }
                }
            }
            ui_context->pop_parent();

            button_with_label(back_button, "Back");
        }
        ui_context->pop_parent();
        ui_context->end(&root);
    }

    virtual void onUpdate(float) override {
        if (Menu::get().state != Menu::State::Settings) return;
        SetExitKey(KEY_NULL);

        // TODO with gamelayer, support events
        if (minimized) {
            return;
        }
    }

    virtual void onDraw(float) override {
        if (Menu::get().state != Menu::State::Settings) return;
        // TODO with gamelayer, support events
        if (minimized) {
            return;
        }

        ClearBackground(ui_context->active_theme().background);
        draw_ui();
    }
};
