#pragma once

#include "../engine.h"
#include "../engine/app.h"
#include "../engine/ui/svg.h"
#include "../engine/util.h"
#include "../external_include.h"
#include "../local_ui.h"

struct SVGRenderer {
    SVGNode root;
    raylib::Texture background_texture;
    std::string svg_name;

    explicit SVGRenderer(const std::string& svg_name) : svg_name(svg_name) {
        // TODO replace with actual file loading
        std::string svg = fmt::format("./resources/ui/{}.svg", svg_name);
        root = load_and_parse(svg);
        background_texture = TextureLibrary::get().get(svg_name);
    }

    void draw_background() {
        float scale = background_texture.width / 1280.f;
        raylib::DrawTextureEx(background_texture, {0, 0}, 0.f, scale, WHITE);
    }

    ui::ElementResult button(const std::string& id,
                             const TranslatableString& content) {
        auto element = SVGNode::find_matching_id(root, id);
        if (!element.has_value()) {
            log_warn("Failed to find {} in svg {}", id, svg_name);
            return ui::ElementResult{false};
        }
        Rectangle rect = element.value().get_and_scale_rect();
        // log_info("id {} @ {} ", id, rect);
        return ui::button(ui::Widget{rect}, content, false);
    }

    ui::ElementResult text(const std::string& id,
                           const TranslatableString& content) {
        auto element = SVGNode::find_matching_id(root, id);
        if (!element.has_value()) {
            log_warn("Failed to find {} in svg {}", id, svg_name);
            return ui::ElementResult{false};
        }
        Rectangle rect = element.value().get_and_scale_rect();
        // log_info("id {} @ {} ", id, rect);
        return ui::text(ui::Widget{rect}, content);
    }
};

struct MenuLayer : public Layer {
    SVGRenderer svg;
    std::shared_ptr<ui::UIContext> ui_context;

    MenuLayer()
        : Layer(strings::menu::MENU),
          svg(SVGRenderer("main_menu")),
          ui_context(std::make_shared<ui::UIContext>()) {}

    virtual ~MenuLayer() {}

    bool onGamepadAxisMoved(GamepadAxisMovedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Root)) return false;
        return ui_context->process_gamepad_axis_event(event);
    }

    bool onKeyPressed(KeyPressedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Root)) return false;
        return ui_context->process_keyevent(event);
    }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Root)) return false;
        return ui_context->process_gamepad_button_event(event);
    }

    void play_music() {
        if (ENABLE_SOUND) {
            // TODO better music playing wrapper so we can not duplicate this
            // everwhere
            // TODO music stops playing when you grab title bar
            auto m = MusicLibrary::get().get(strings::music::THEME);
            if (!raylib::IsMusicStreamPlaying(m)) {
                raylib::PlayMusicStream(m);
            }
            raylib::UpdateMusicStream(m);
        }
    }

    virtual void onUpdate(float) override {
        if (MenuState::get().is_in_menu()) play_music();
        if (MenuState::get().is_not(menu::State::Root)) return;
        raylib::SetExitKey(raylib::KEY_ESCAPE);
    }

    virtual void onDraw(float dt) override {
        if (MenuState::get().is_not(menu::State::Root)) return;
        ext::clear_background(ui::UI_THEME.background);
        using namespace ui;

        begin(ui_context, dt);

        svg.draw_background();

        svg.text("Title", TranslatableString(strings::GAME_NAME));

        if (svg.button("PlayButton", TranslatableString(strings::i18n::PLAY))) {
            MenuState::get().set(menu::State::Network);
        }
        if (svg.button("AboutButton",
                       TranslatableString(strings::i18n::ABOUT))) {
            MenuState::get().set(menu::State::About);
        }

        if (svg.button("SettingsButton",
                       TranslatableString(strings::i18n::SETTINGS))) {
            MenuState::get().set(menu::State::Settings);
        }

        if (svg.button("ExitButton", TranslatableString(strings::i18n::EXIT))) {
            App::get().close();
        }

        if (svg.button("discord", NO_TRANSLATE(""))) {
            util::open_url(strings::urls::DISCORD);
        }

        if (svg.button("itch-white", NO_TRANSLATE(""))) {
            util::open_url(strings::urls::ITCH);
        }

        end();
    }
};
