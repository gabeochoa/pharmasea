#pragma once

#include "../engine.h"
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

    bool windowSizeDropdownState = false;
    int windowSizeDropdownIndex = 0;
    bool resolution_dropdown_open = false;
    int resolution_selected_index = 0;

    bool language_dropdown_open = false;
    int language_selected_index = 0;

    SettingsLayer()
        : Layer("Settings"),
          ui_context(std::make_shared<ui::UIContext>()),
          root_box(load_ui("resources/html/settings.html", WIN_R())) {
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

    void streamer_safe_box() {
        auto container = ui_context->own(
            Widget({.mode = Children}, {.mode = Children}, GrowFlags::Row));

        div(*container);
        ui_context->push_parent(container);
        {
            text(*ui::components::mk_text(),
                 text_lookup(strings::i18n::SHOW_SAFE_BOX));
            auto checkbox_widget = ui_context->own(Widget(
                MK_UUID(id, ROOT_ID), Size_Px(75.f, 0.5f), Size_Px(25.f, 1.f)));
            bool sssb = Settings::get().data.show_streamer_safe_box;
            if (checkbox(*checkbox_widget, &sssb)) {
                Settings::get().update_streamer_safe_box(sssb);
            }
        }
        ui_context->pop_parent();
    }

    void enable_post_processing() {
        auto container = ui_context->own(
            Widget({.mode = Children}, {.mode = Children}, GrowFlags::Row));

        div(*container);
        ui_context->push_parent(container);
        {
            text(*ui::components::mk_text(),
                 text_lookup(strings::i18n::ENABLE_PPS));
            auto checkbox_widget = ui_context->own(Widget(
                MK_UUID(id, ROOT_ID), Size_Px(75.f, 0.5f), Size_Px(25.f, 1.f)));
            bool sssb = Settings::get().data.enable_postprocessing;
            if (checkbox(*checkbox_widget, &sssb)) {
                Settings::get().update_post_processing_enabled(sssb);
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
            text(*ui::components::mk_text(),
                 text_lookup(strings::i18n::MASTER_VOLUME));
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
            text(*ui::components::mk_text(),
                 text_lookup(strings::i18n::MUSIC_VOLUME));
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
            text(*ui::components::mk_text(),
                 text_lookup(strings::i18n::RESOLUTION));
            padding(*ui::components::mk_padding(Size_Px(100.f, 1.f),
                                                Size_Px(100.f, 1.f)));

            auto dropdown_widget =
                ui_context->own(Widget({Size_Px(100.f, 1.f), Size_Px(50.f, 1.f),
                                        GrowFlags::Row | GrowFlags::Column}));

            if (dropdown(*dropdown_widget, Settings::get().resolution_options(),
                         &resolution_dropdown_open,
                         &resolution_selected_index)) {
                Settings::get().update_resolution_from_index(
                    resolution_selected_index);
            }

            ui_context->pop_parent();
        }
    }

    void language_switcher() {
        auto language_switcher_container = ui_context->own(
            Widget({.mode = Children}, {.mode = Children}, GrowFlags::Row));

        div(*language_switcher_container);
        ui_context->push_parent(language_switcher_container);
        {
            text(*ui::components::mk_text(),
                 text_lookup(strings::i18n::LANGUAGE));
            padding(*ui::components::mk_padding(Size_Px(100.f, 1.f),
                                                Size_Px(100.f, 1.f)));

            auto dropdown_widget =
                ui_context->own(Widget({Size_Px(100.f, 1.f), Size_Px(50.f, 1.f),
                                        GrowFlags::Row | GrowFlags::Column}));

            if (dropdown(*dropdown_widget, Settings::get().language_options(),
                         &language_dropdown_open, &language_selected_index)) {
                Settings::get().update_language_from_index(
                    language_selected_index);
            }

            ui_context->pop_parent();
        }
    }

    void back_button() {
        if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID),
                                              Size_Px(120.f, 1.f),
                                              Size_Px(50.f, 1.f)),
                   text_lookup(strings::i18n::BACK_BUTTON))) {
            MenuState::get().go_back();
        }
        padding(*ui::components::mk_padding(Size_Px(100.f, 1.f),
                                            Size_Pct(0.5f, 0.f)));
    }

    void draw_root_settings(float) {
        padding(*ui::components::mk_padding(Size_Px(100.f, 1.f),
                                            Size_Px(100.f, 0.5f)));
        master_volume();
        music_volume();
        // TODO
        // resolution_switcher();
        language_switcher();
        streamer_safe_box();
        enable_post_processing();
        back_button();
    }

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

    void draw_ui(float dt) {
        using namespace ui;

        ui_context->begin(dt);

        auto root = ui::components::mk_root();

        ui_context->push_parent(root);
        {
            padding(*ui::components::mk_padding(Size_Px(100.f, 1.f),
                                                Size_Px(WIN_HF(), 1.f)));
            auto content =
                ui_context->own(Widget({.mode = Children, .strictness = 1.f},
                                       Size_Pct(1.f, 1.f), Column));
            div(*content);
            ui_context->push_parent(content);
            {
                switch (activeWindow) {
                    default:
                    case ActiveWindow::Root:
                        draw_root_settings(dt);
                        break;
                    case ActiveWindow::KeyBindings:
                        draw_keybindings(dt);
                        break;
                }
            }
            ui_context->pop_parent();
        }
        ui_context->pop_parent();

        ui_context->end(root.get());
    }

    virtual void onUpdate(float) override {
        if (MenuState::get().is_not(menu::State::Settings)) return;
        raylib::SetExitKey(raylib::KEY_NULL);
    }

    void process_on_click(const std::string& id) {
        log_info("clicked {}", id);
        switch (hashString(id)) {
            case hashString(strings::i18n::BACK_BUTTON):
                MenuState::get().set(menu::State::Root);
                break;
        }
    }

    elements::InputDataSource dataFetcher(const std::string& id) {
        switch (hashString(id)) {
            case hashString("LanguageSwitcher"):
                return Settings::get().language_options();
        }
        return "";
    }

    virtual void onDraw(float dt) override {
        if (MenuState::get().is_not(menu::State::Settings)) return;
        ext::clear_background(ui_context->active_theme().background);
        // draw_ui(dt);
        elements::begin(ui_context);

        render_ui(root_box, WIN_R(),
                  std::bind(&SettingsLayer::process_on_click, *this,
                            std::placeholders::_1),
                  std::bind(&SettingsLayer::dataFetcher, *this,
                            std::placeholders::_1));

        elements::end();
    }
};
