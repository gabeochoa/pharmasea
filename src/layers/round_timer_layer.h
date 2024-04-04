
#pragma once

#include "../components/has_timer.h"
#include "../components/is_bank.h"
#include "../components/is_spawner.h"
#include "../entity.h"
#include "../entity_helper.h"
#include "../entity_query.h"
#include "base_game_renderer.h"

struct RoundTimerLayer : public BaseGameRendererLayer {
    float _pct = 0.f;

    RoundTimerLayer() : BaseGameRendererLayer("RoundTimer") {}

    virtual ~RoundTimerLayer() {}

    virtual bool shouldSkipRender() override {
        if (!GameState::get().should_render_timer()) return true;
        return false;
    }

    /*
        auto [top, middle, bottom] = rect::hsplit<3>(window, 20);

        auto [top_a, top_b, top_c] = rect::vsplit<3>(top, 0);
        auto [middle_a, middle_b, middle_c] = rect::vsplit<3>(middle, 0);
        auto [bottom_a, bottom_b, bottom_c] = rect::vsplit<3>(bottom, 0);
    */

    OptEntity get_timer_entity() {
        return EntityQuery().whereHasComponent<HasTimer>().gen_first();
    }

    TranslatedString get_status_text(const HasTimer& ht) {
        const bool is_closing = ht.store_is_closed();
        const bool is_day = GameState::get().in_round() && !is_closing;
        const int dayCount = ht.dayCount;

        auto status_text =
            is_day ? text_lookup(strings::i18n::OPEN)
                   : (is_closing ? text_lookup(strings::i18n::CLOSING)
                                 : text_lookup(strings::i18n::CLOSED));
        // TODO figure out how to translate strings that have numbers in
        // them...
        auto day_text = fmt::format(
            "{} {}", text_lookup(strings::i18n::ROUND_DAY).underlying,
            dayCount);
        return TODO_TRANSLATE(
            fmt::format("{} {}", status_text.underlying, day_text),
            TodoReason::Format);
    }

    void animate_new_transaction(const IsBank::Transaction& transaction,
                                 Rectangle spawn_count) {
        if (transaction.amount == 0 && transaction.extra == 0) return;

        spawn_count.y += static_cast<int>(60 * transaction.pct());

        bool positive = transaction.amount >= 0;
        unsigned char alpha =
            static_cast<unsigned char>(255 * transaction.pct());

        const auto tip_string =
            fmt::format("{} {}", transaction.extra,
                        text_lookup(strings::i18n::STORE_TIP).underlying);

        colored_text(
            ui::Widget{spawn_count},
            fmt::format("          {}{} {}",
                        positive ? "+" : "-",  //
                        transaction.amount,
                        transaction.extra ? tip_string : ""),
            positive ? Color{0, 255, 0, alpha} : Color{255, 0, 0, alpha}  //
        );
    }

    virtual void onDrawUI(float) override {
        using namespace ui;
        // not putting these in shouldSkip since we expect this to exist like
        // 99% of the time
        auto entity = get_timer_entity();
        if (!entity) return;
        const HasTimer& ht = entity->get<HasTimer>();
        if (ht.type != HasTimer::Renderer::Round) return;

        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        window = rect::tpad(window, 10);
        window = rect::rpad(window, 20);
        window = rect::lpad(window, 10);
        window = rect::bpad(window, 20);

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

        OptEntity sophie =
            EntityQuery().whereType(EntityType::Sophie).gen_first();
        {
            Rectangle spawn_count(rounded_rect);
            spawn_count.y += 60;

            if (sophie.valid()) {
                const IsBank& bank = sophie->get<IsBank>();
                text(Widget{spawn_count},
                     TODO_TRANSLATE(
                         fmt::format("{}: {}",
                                     text_lookup(strings::i18n::STORE_BALANCE)
                                         .underlying,
                                     bank.balance()),
                         TodoReason::Format));

                const std::vector<IsBank::Transaction>& transactions =
                    bank.get_transactions();
                if (!transactions.empty()) {
                    const IsBank::Transaction& transaction =
                        transactions.front();
                    animate_new_transaction(transaction, spawn_count);
                }
            }
        }

        // Only show the customer count during planning
        if (GameState::get().is(game::State::Planning)) {
            Rectangle spawn_count(rounded_rect);
            spawn_count.y += 120;

            Entity& spawner = (EntityQuery()
                                   .whereType(EntityType::CustomerSpawner)
                                   .gen_first())
                                  .asE();
            const IsSpawner& iss = spawner.get<IsSpawner>();
            text(Widget{spawn_count},
                 TODO_TRANSLATE(
                     fmt::format(
                         "{}: {}",
                         text_lookup(strings::i18n::PLANNING_CUSTOMERS_COMING)
                             .underlying,
                         iss.get_max_spawned()),
                     TodoReason::Format));
        }
    }
};
