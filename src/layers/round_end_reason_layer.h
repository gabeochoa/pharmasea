
#pragma once

#include "base_game_renderer.h"

struct RoundEndReasonLayer : public BaseGameRendererLayer {
    std::shared_ptr<ui::UIContext> ui_context;

    RoundEndReasonLayer() : BaseGameRendererLayer("RoundEndReason") {}

    virtual ~RoundEndReasonLayer() {}

    OptEntity get_timer_entity() {
        return EntityHelper::getFirstWithComponent<HasTimer>();
    }

    virtual bool shouldSkipRender() override { return !shouldRender(); }

    bool shouldRender() {
        if (!GameState::get().should_render_timer()) return false;

        auto entity = get_timer_entity();
        if (!entity) return false;
        const HasTimer& ht = entity->get<HasTimer>();
        if (ht.type != HasTimer::Renderer::Round) return false;

        const auto debug_mode_on =
            GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
        if (debug_mode_on) return true;

        if (ht.currentRoundTime <= 0) return true;

        if (GameState::get().is(game::State::Planning) &&
            ht.block_state_change_reasons.any()) {
            return true;
        }
        return false;
    }

    virtual void onDrawUI(float) override {
        using namespace ui;

        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        auto content = rect::tpad(window, 20);
        content = rect::lpad(content, 20);
        content = rect::rpad(content, 75);

        auto entity = get_timer_entity();
        if (!entity) return;
        const HasTimer& ht = entity->get<HasTimer>();
        if (ht.type != HasTimer::Renderer::Round) return;

        if (ht.block_state_change_reasons.none()) {
            text(Widget{content},
                 fmt::format(
                     "{}: {}", text_lookup(strings::i18n::NEXT_ROUND_COUNTDOWN),
                     (int) ceil(util::trunc(ht.roundSwitchCountdown, 1))),
                 theme::Usage::Font, true);
            return;
        }

        auto reason_spots =
            rect::hsplit<HasTimer::WaitingReason::WaitingReasonLast>(content,
                                                                     10);

        std::vector<std::string> active_reasons;
        for (int i = 1; i < HasTimer::WaitingReason::WaitingReasonLast; i++) {
            if (ht.read_reason(i)) {
                active_reasons.push_back(ht.text_reason(i));
            }
        }

        int i = 0;
        for (auto txt : active_reasons) {
            text(Widget{reason_spots[i]}, active_reasons[i], theme::Usage::Font,
                 true);
            i++;
        }
    }
};
