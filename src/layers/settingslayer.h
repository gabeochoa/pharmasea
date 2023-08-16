#pragma once

#include "../engine.h"
#include "../engine/settings.h"
#include "../engine/ui/ui.h"
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
    } activeWindow = ActiveWindow::Root;
    Trie keyBindingTrie;
    std::array<std::pair<InputName, std::string_view>,
               magic_enum::enum_count<InputName>()>
        keyInputNames;
    std::vector<std::pair<InputName, AnyInputs>> keyInputValues;

    std::shared_ptr<ui::UIContext> ui_context;
    LayoutBox root_box;

    SettingsLayer()
        : Layer("Settings"),
          ui_context(std::make_shared<ui::UIContext>()),
          root_box(load_ui("resources/html/settings.html", WIN_R())) {
        ui_context = std::make_shared<ui::UIContext>();

        keyInputNames = magic_enum::enum_entries<InputName>();
        for (const auto& kv : keyInputNames) {
            keyBindingTrie.add(std::string(kv.second));
            keyInputValues.push_back(std::make_pair(
                kv.first,
                KeyMap::get_valid_inputs(menu::State::Game, kv.first)));
        }
    }

    virtual ~SettingsLayer() {}

    bool onKeyPressed(KeyPressedEvent& event) override {
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

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Settings)) return false;
        return ui_context.get()->process_gamepad_button_event(event);
    }

    /*

    void draw_keybindings(float) {
        // TODO tons more work to do here but for the most part this shows its
        // possible to load all of them

        // TODO save all the changed inputs
        for (const auto& kv : keyInputNames) {
            const auto keys =
                KeyMap::get_valid_keys(menu::State::Game, kv.first);
            // TODO handle multiple keys
            if (keys.empty()) continue;
            auto opt_key = magic_enum::enum_cast<raylib::KeyboardKey>(keys[0]);
            if (!opt_key.has_value()) continue;
            raylib::KeyboardKey kbKey = opt_key.value();

            if (button(*ui::components::mk_button(
                           MK_UUID_LOOP(id, ROOT_ID, kv.first)),
                       fmt::format("{}({})", kv.second,
                                   magic_enum::enum_name(kbKey)))) {
                std::cout << "HI " << kv.second << std::endl;
            }
        }
    }
    */

    virtual void onUpdate(float) override {
        if (MenuState::get().is_not(menu::State::Settings)) return;
        raylib::SetExitKey(raylib::KEY_NULL);
    }

    void process_on_click(const std::string& id) {
        switch (hashString(id)) {
            case hashString(strings::i18n::BACK_BUTTON):
                MenuState::get().go_back();
                break;
        }
    }

    elements::InputDataSource dataFetcher(const std::string& id) {
        switch (hashString(id)) {
            case hashString("MasterVolume"): {
                return Settings::get().data.master_volume;
            } break;
            case hashString("MusicVolume"): {
                return Settings::get().data.music_volume;
            } break;
            case hashString("LanguageSwitcher"):
                return elements::DropdownData{
                    Settings::get().language_options(),
                    Settings::get().get_current_language_index(),
                };
            case hashString("ResolutionSwitcher"):
                return elements::DropdownData{
                    Settings::get().resolution_options(),
                    Settings::get().get_current_resolution_index(),
                };
            case hashString("StreamerSafeBox"):
                return Settings::get().data.show_streamer_safe_box;
            case hashString("PostPressingEffects"):
                return Settings::get().data.enable_postprocessing;
        }
        return "";
    }

    void inputProcessor(const std::string& id, elements::ElementResult result) {
        switch (hashString(id)) {
            case hashString("MasterVolume"): {
                Settings::get().update_master_volume(result.as<float>());
            } break;
            case hashString("MusicVolume"): {
                Settings::get().update_music_volume(result.as<float>());
            } break;
            case hashString("LanguageSwitcher"): {
                Settings::get().update_language_from_index(result.as<int>());
            } break;
            case hashString("ResolutionSwitcher"): {
                Settings::get().update_resolution_from_index(result.as<int>());
            } break;
            case hashString("StreamerSafeBox"): {
                Settings::get().update_streamer_safe_box(result.as<bool>());
            } break;
            case hashString("PostProcessingEffects"): {
                Settings::get().update_post_processing_enabled(
                    result.as<bool>());
            } break;
        }
        return;
    }

    virtual void onDraw(float dt) override {
        if (MenuState::get().is_not(menu::State::Settings)) return;
        ext::clear_background(ui_context->active_theme().background);

        elements::begin(ui_context, dt);

        render_ui(root_box, WIN_R(),
                  std::bind(&SettingsLayer::process_on_click, *this,
                            std::placeholders::_1),
                  std::bind(&SettingsLayer::dataFetcher, *this,
                            std::placeholders::_1),
                  std::bind(&SettingsLayer::inputProcessor, *this,
                            std::placeholders::_1, std::placeholders::_2));

        elements::end();
    }
};
