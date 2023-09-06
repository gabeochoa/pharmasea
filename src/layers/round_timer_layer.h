
#pragma once

#include "../components/has_timer.h"
#include "base_game_renderer.h"

struct RoundTimerLayer : public BaseGameRendererLayer {
    std::shared_ptr<ui::UIContext> ui_context;
    float _pct = 0.f;

    RoundTimerLayer() : BaseGameRendererLayer("RoundTimer") {}

    virtual ~RoundTimerLayer() {}

    virtual bool shouldSkipRender() override {
        return !GameState::get().should_render_timer();
    }

    /*
        auto [top, middle, bottom] = rect::hsplit<3>(window, 20);

        auto [top_a, top_b, top_c] = rect::vsplit<3>(top, 0);
        auto [middle_a, middle_b, middle_c] = rect::vsplit<3>(middle, 0);
        auto [bottom_a, bottom_b, bottom_c] = rect::vsplit<3>(bottom, 0);
    */

    OptEntity get_timer_entity() {
        return EntityHelper::getFirstWithComponent<HasTimer>();
    }

    std::string get_status_text(const HasTimer& ht) {
        const bool is_closing = ht.store_is_closed();
        const bool is_day = GameState::get().in_round() && !is_closing;
        const int dayCount = ht.dayCount;

        auto status_text =
            is_day ? text_lookup(strings::i18n::OPEN)
                   : (is_closing ? text_lookup(strings::i18n::CLOSING)
                                 : text_lookup(strings::i18n::CLOSED));
        // TODO figure out how to translate strings that have numbers in
        // them...
        auto day_text = fmt::format("{} {}", "Day", dayCount);
        return std::string(fmt::format("{} {}", status_text, day_text));
    }

    virtual void onDrawUI(float) override {
        using namespace ui;

        // TODO move to shouldSkip?
        auto entity = get_timer_entity();
        if (!entity) return;
        const HasTimer& ht = entity->get<HasTimer>();
        if (ht.type != HasTimer::Renderer::Round) return;

        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        window = rect::tpad(window, 10);
        window = rect::rpad(window, 20);
        window = rect::bpad(window, 20);
        window = rect::lpad(window, 10);

        const vec2 center = {
            window.x + (window.width / 2.f),
            window.y + (window.height / 2.f),
        };

        const bool is_closing = ht.store_is_closed();
        const bool is_day = GameState::get().in_round() && !is_closing;
        const float pct = ht.pct();

        const float radius = WIN_WF() / 64.f;
        const float r10 = radius * 6;
        const float angle = util::deg2rad(util::lerp(170, 365, 1 - pct));
        const vec2 pos = {
            center.x + std::cos(angle) * (r10),
            center.y + std::sin(angle) * (r10),
        };

        // Hide it when its below the rect
        if (angle >= M_PI)
            raylib::DrawCircle((int) pos.x, (int) pos.y, radius,
                               is_day ? YELLOW : GRAY);

        const vec2 start = {center.x - r10 - (radius), center.y};
        const vec2 end = {center.x + r10 + (radius), center.y};
        Rectangle rounded_rect = {start.x,          //
                                  start.y - 10,     //
                                  end.x - start.x,  //
                                  radius * 2.f};

        div(Widget{rounded_rect}, is_day ? theme::Primary : theme::Background);
        text(Widget{rounded_rect}, get_status_text(ht));
    }
};
