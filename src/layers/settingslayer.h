#pragma once

#include "../engine.h"
#include "../engine/app.h"
#include "../engine/settings.h"
#include "../engine/ui.h"
#include "../external_include.h"

using namespace ui;

// TODO add support for customizing keymap

// TODO is there a way to automatically build this UI based on the
// settings::data format?
// (though some settings probably dont need to show)

struct SettingsLayer : public Layer {
    // TODO add way to get into keybindings mode
    enum ActiveWindow {
        Root = 0,
        KeyBindings = 1,
    } activeWindow = ActiveWindow::KeyBindings;

    InputType selected_input_type = InputType::Keyboard;

    Trie keyBindingTrie;
    std::array<std::pair<InputName, std::string_view>,
               magic_enum::enum_count<InputName>()>
        keyInputNames;
    std::vector<std::pair<InputName, AnyInputs>> keyInputValues;

    std::shared_ptr<ui::UIContext> ui_context;
    bool windowSizeDropdownState = false;
    int windowSizeDropdownIndex = 0;
    bool resolution_dropdown_open = false;
    int resolution_selected_index = 0;

    bool language_dropdown_open = false;
    int language_selected_index = 0;

    SettingsLayer() : Layer("Settings") {
        ui_context = std::make_shared<ui::UIContext>();

        resolution_selected_index =
            Settings::get().get_current_resolution_index();
        if (resolution_selected_index < 0) resolution_selected_index = 0;

        language_selected_index = Settings::get().get_current_language_index();
        if (language_selected_index < 0) language_selected_index = 0;

        keyInputNames = magic_enum::enum_entries<InputName>();
        for (const auto& kv : keyInputNames) {
            keyBindingTrie.add(std::string(kv.second));
            keyInputValues.push_back(std::make_pair(
                kv.first,
                KeyMap::get_valid_inputs(menu::State::Game, kv.first)));
        }
    }

    virtual ~SettingsLayer() {}

    virtual bool onKeyPressed(KeyPressedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Settings)) return false;

        //
        if (event.keycode == raylib::KEY_ESCAPE &&
            activeWindow != ActiveWindow::Root) {
            activeWindow = ActiveWindow::Root;
            return true;
        }

        if (event.keycode == raylib::KEY_ESCAPE) {
            MenuState::get().go_back();
            return true;
        }
        return ui_context.get()->process_keyevent(event);
    }

    virtual bool onGamepadButtonPressed(
        GamepadButtonPressedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Settings)) return false;
        return ui_context.get()->process_gamepad_button_event(event);
    }

    virtual void onUpdate(float) override {
        if (MenuState::get().is_not(menu::State::Settings)) return;
        raylib::SetExitKey(raylib::KEY_NULL);
    }

    virtual void onDraw(float dt) override {
        if (MenuState::get().is_not(menu::State::Settings)) return;
        ext::clear_background(ui_context->active_theme().background);

        begin(ui_context, dt);

        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        auto content = rect::tpad(window, 20);
        content = rect::lpad(content, 10);

        auto [rows, footer] = rect::hsplit(content, 80);
        footer = rect::bpad(footer, 50);

        {
            auto [master_vol, music_vol, resolution, language, streamer,
                  postprocessing] = rect::hsplit<6>(rect::bpad(rows, 85), 20);

            {
                auto [label, control] = rect::vsplit(master_vol, 30);
                control = rect::rpad(control, 30);

                text(Widget{label}, text_lookup(strings::i18n::MASTER_VOLUME));
                if (auto result =
                        slider(Widget{control},
                               {.value = Settings::get().data.master_volume});
                    result) {
                    Settings::get().update_master_volume(result.as<float>());
                }
            }

            {
                auto [label, control] = rect::vsplit(music_vol, 30);
                control = rect::rpad(control, 30);

                text(Widget{label}, text_lookup(strings::i18n::MUSIC_VOLUME));
                if (auto result =
                        slider(Widget{control},
                               {.value = Settings::get().data.music_volume});
                    result) {
                    Settings::get().update_music_volume(result.as<float>());
                }
            }

            {
                auto [label, control] = rect::vsplit(resolution, 30);
                control = rect::rpad(control, 30);

                text(Widget{label}, text_lookup(strings::i18n::RESOLUTION));
                if (auto result = dropdown(
                        Widget{control},
                        DropdownData{
                            Settings::get().resolution_options(),
                            Settings::get().get_current_resolution_index(),
                        });
                    result) {
                    Settings::get().update_resolution_from_index(
                        result.as<int>());
                }
            }

            {
                auto [label, control] = rect::vsplit(language, 30);
                control = rect::rpad(control, 30);

                text(Widget{label}, text_lookup(strings::i18n::LANGUAGE));
                if (auto result = dropdown(
                        Widget{control},
                        DropdownData{
                            Settings::get().language_options(),
                            Settings::get().get_current_language_index(),
                        });
                    result) {
                    Settings::get().update_language_from_index(
                        result.as<int>());
                }
            }

            {
                auto [label, control] = rect::vsplit(streamer, 30);
                control = rect::rpad(control, 10);

                text(Widget{label}, text_lookup(strings::i18n::SHOW_SAFE_BOX));

                // TODO default value wont be setup correctly without this
                // bool sssb = Settings::get().data.show_streamer_safe_box;
                if (auto result = checkbox(
                        Widget{control},
                        CheckboxData{
                            .selected =
                                Settings::get().data.show_streamer_safe_box});
                    result) {
                    Settings::get().update_streamer_safe_box(result.as<bool>());
                }
            }

            {
                auto [label, control] = rect::vsplit(postprocessing, 30);
                control = rect::rpad(control, 10);

                text(Widget{label}, text_lookup(strings::i18n::ENABLE_PPS));

                // TODO default value wont be setup correctly without this
                // bool sssb = Settings::get().data.enable_postprocessing;
                if (auto result = checkbox(
                        Widget{control},
                        CheckboxData{
                            .selected =
                                Settings::get().data.enable_postprocessing});
                    result) {
                    Settings::get().update_post_processing_enabled(
                        result.as<bool>());
                }
            }
        }

        // Footer
        {
            footer = rect::rpad(footer, 30);
            if (button(Widget{footer}, text_lookup(strings::i18n::BACK_BUTTON),
                       true)) {
                MenuState::get().go_back();
            }
        }

        end();
    }
};
