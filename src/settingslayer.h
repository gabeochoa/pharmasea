#pragma once

#include "event.h"
#include "external_include.h"
//
#include "layer.h"
#include "menu.h"
#include "settings.h"
#include "ui.h"
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

    void window_size_dropdown() {
        using namespace ui;
        // TODO select the acurate options based on current settings
        auto& settings = Settings::get();

        std::vector<WidgetConfig> dropdownConfigs;
        dropdownConfigs.push_back(ui::WidgetConfig({.text = "960x540"}));
        dropdownConfigs.push_back(ui::WidgetConfig({.text = "1920x1080"}));
        dropdownConfigs.push_back(ui::WidgetConfig({.text = "800x600"}));

        auto theme = WidgetTheme();
        text(MK_UUID(id, ROOT_ID), WidgetConfig({
                                       .position = vec2{25.f, 150.f},
                                       .size = vec2{30.f, 5.f},
                                       .text = "Resolution",
                                   }));

        WidgetConfig dropdownMain =
            WidgetConfig({.position = vec2{200.f, 150.f},  //
                          .size = vec2{100.f, 30.f},       //
                          .text = "",                      //
                          .theme = theme});

        if (ui::dropdown(ui::MK_UUID(id, ui::ROOT_ID), dropdownMain,
                         dropdownConfigs, &windowSizeDropdownState,
                         &windowSizeDropdownIndex)) {
            // std::cout << "dropdown selected: ";
            // std::cout << dropdownConfigs[windowSizeDropdownIndex].text;
            // std::cout << std::endl;
            if (windowSizeDropdownIndex == 0) {
                settings.update_window_size({960, 540});
            } else if (windowSizeDropdownIndex == 1) {
                settings.update_window_size({1920, 1080});
            } else if (windowSizeDropdownIndex == 2) {
                settings.update_window_size({800, 600});
            }
        }
    }

    void back_button() {
        using namespace ui;
        if (button(MK_UUID(id, ROOT_ID), WidgetConfig({
                                             .position = vec2{25.f, 300.f},
                                             .size = vec2{100.f, 50.f},
                                             .text = std::string("Back"),
                                         }))) {
            Menu::get().state = Menu::State::Root;
        }
    }

    void volume_sliders() {
        using namespace ui;
        text(MK_UUID(id, ROOT_ID), WidgetConfig({
                                       .position = vec2{25.f, 100.f},
                                       .size = vec2{30.f, 5.f},
                                       .text = "Master Volume",
                                   }));

        if (slider(MK_UUID(id, ROOT_ID),
                   WidgetConfig({
                       .position = vec2{200.f, 100.f},
                       .size = vec2{150.f, 25.f},
                       .vertical = false,
                   }),
                   &masterVolumeSliderValue, 0.f, 1.f)) {
            Settings::get().update_master_volume(masterVolumeSliderValue);
        }
    }

    void draw_ui() {
        using namespace ui;

        // TODO move to input
        bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        vec2 mousepos = GetMousePosition();

        ui_context->begin(mouseDown, mousepos);

        volume_sliders();
        window_size_dropdown();
        back_button();

        ui_context->end();
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

        ClearBackground(Color{30, 30, 30, 255});
        draw_ui();
    }
};
