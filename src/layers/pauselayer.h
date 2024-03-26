
#pragma once

#include "../engine.h"
#include "../engine/layer.h"
#include "../engine/ui/ui.h"
#include "../external_include.h"
#include "../network/network.h"

//
#include "../components/is_round_settings_manager.h"
#include "../entity_query.h"

using namespace ui;

struct BasePauseLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;
    game::State enabled_state;

    BasePauseLayer(const char* name, game::State e_state)
        : Layer(name), enabled_state(e_state) {
        ui_context = std::make_shared<ui::UIContext>();
    }
    virtual ~BasePauseLayer() {}

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) override {
        if (GameState::get().is_not(enabled_state)) return false;
        if (KeyMap::get_button(menu::State::Game, InputName::Pause) ==
            event.button) {
            GameState::get().go_back();
            return true;
        }
        return ui_context->process_gamepad_button_event(event);
    }

    bool onKeyPressed(KeyPressedEvent& event) override {
        if (GameState::get().is_not(enabled_state)) return false;
        if (KeyMap::get_key_code(menu::State::Game, InputName::Pause) ==
            event.keycode) {
            GameState::get().go_back();
            return true;
        }
        return ui_context->process_keyevent(event);
    }

    virtual void onUpdate(float) override {}

    void draw_upgrades(Rectangle window, Rectangle rect) {
        OptEntity sophie = EntityQuery()
                               .whereHasComponent<IsRoundSettingsManager>()
                               .gen_first();
        if (!sophie) return;
        /*

        const IsRoundSettingsManager& irsm =
            sophie->get<IsRoundSettingsManager>();
        // TODO right now we show all unlocked upgrades is that fine?
        // how to distiguish the ones that are active
        const auto unlocked_upgrades = irsm.unlocked_upgrades;

        if (unlocked_upgrades.empty()) return;

        if (unlocked_upgrades.size() > 100) {
            log_warn("More upgrades than we can display");
        }

        auto upgrades = rect::bpad(window, 90);

        std::vector<Rectangle> rects;
        upgrades = rect::lpad(upgrades, 10);
        upgrades = rect::rpad(upgrades, 90);
        auto rows = rect::hsplit<10>(upgrades, 20);
        for (auto r : rows) {
            auto cols = rect::vsplit<10>(r, 20);
            for (auto c : cols) {
                rects.push_back(c);
            }
        }

        int i = 0;

        std::optional<Upgrade> hovered_upgrade = {};

        for (const auto& name : unlocked_upgrades) {
            if (i > (int) rects.size()) break;
            const Upgrade& upgrade = UpgradeLibrary::get().get(name);

            Widget icon = Widget{rects[i]};

            if (irsm.is_upgrade_active(name)) {
                div(icon, ui::theme::Usage::Primary);
            }

            image(icon, upgrade.icon_name);
            if (hoverable(icon)) {
                hovered_upgrade = upgrade;
            }
            i++;
        }

        if (hovered_upgrade) {
            div(rect, ui::theme::Usage::Background);

            const auto [header, rest] = rect::hsplit<2>(rect);

            const auto icon = rect::rpad(header, 80);
            const auto name = rect::lpad(header, 10);
            image(icon, hovered_upgrade->icon_name);
            text(name, hovered_upgrade->name);

            const auto [flavor, desc] = rect::hsplit<2>(rest);
            text(flavor, hovered_upgrade->flavor_text);
            text(desc, hovered_upgrade->description);
        }
        */
    }

    virtual void onDraw(float dt) override {
        // Note: theres no pausing outside the game, so dont render
        if (!MenuState::s_in_game()) return;

        if (GameState::get().is_not(enabled_state)) return;

        // NOTE: We specifically dont clear background
        // because people are used to pause menu being an overlay

        using namespace ui;
        begin(ui_context, dt);

        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};

        auto content = rect::tpad(window, 33);
        content = rect::bpad(content, 66);
        content = rect::lpad(content, 5);
        content = rect::rpad(content, 80);

        auto body = rect::rpad(content, 30);
        auto upgrades = rect::lpad(content, 20);

        draw_upgrades(window, upgrades);

        body = rect::rpad(body, 60);
        auto [continue_button, settings, config, quit] =
            rect::hsplit<4>(body, 20);

        if (button(Widget{continue_button},
                   text_lookup(strings::i18n::CONTINUE))) {
            GameState::get().go_back();
        }
        if (button(Widget{settings}, text_lookup(strings::i18n::SETTINGS))) {
            MenuState::get().set(menu::State::Settings);
        }
        if (button(Widget{config}, "RELOAD CONFIGS")) {
            Preload::get().reload_config();
        }
        if (button(Widget{quit}, text_lookup(strings::i18n::QUIT))) {
            network::Info::reset_connections();
            return;
        }
        end();
    }
};

struct PauseLayer : public BasePauseLayer {
    PauseLayer() : BasePauseLayer("Pause", game::State::Paused) {}
    virtual ~PauseLayer() {}
};
