
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

    void draw_upgrades(Rectangle rect) {
        OptEntity sophie = EntityQuery()
                               .whereHasComponent<IsRoundSettingsManager>()
                               .gen_first();
        if (!sophie) return;

        const IsRoundSettingsManager& irsm =
            sophie->get<IsRoundSettingsManager>();
        if (irsm.upgrades_applied.empty()) return;

        std::vector<Rectangle> rects;
        auto cols = rect::vsplit<5>(rect::lpad(rect, 10));
        for (auto c : cols) {
            auto rows = rect::hsplit<5>(c);
            for (auto r : rows) {
                rects.push_back(r);
            }
        }

        int i = 0;

        for (const auto& name : irsm.upgrades_applied) {
            if (i > (int) rects.size()) break;
            const Upgrade& upgrade = UpgradeLibrary::get().get(name);
            image(Widget{rects[i]}, upgrade.icon_name);
            i++;
        }
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
        content = rect::lpad(content, 5);
        content = rect::rpad(content, 80);
        content = rect::bpad(content, 66);

        auto [body, upgrades, _a] = rect::vsplit<3>(content, 10);

        draw_upgrades(upgrades);

        body = rect::rpad(body, 90);
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
