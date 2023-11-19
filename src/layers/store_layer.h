#pragma once

#include "../components/has_timer.h"
#include "../components/is_bank.h"
#include "../components/is_spawner.h"
#include "../entity.h"
#include "../entity_helper.h"
#include "../entity_query.h"
#include "base_game_renderer.h"

struct StoreLayer : public BaseGameRendererLayer {
    float _pct = 0.f;

    StoreLayer() : BaseGameRendererLayer("Store") {}

    virtual ~StoreLayer() {}

    virtual bool shouldSkipRender() override {
        if (!GameState::get().is(game::State::Store)) return true;
        return false;
    }

    void render_balances(Rectangle left_col) {
        using namespace ui;
        OptEntity sophie =
            EntityQuery().whereType(EntityType::Sophie).gen_first();
        if (!sophie.valid()) return;

        const IsBank& bank = sophie->get<IsBank>();
        int balance = bank.balance();
        int cart = bank.cart();

        {
            Rectangle spawn_count(left_col);
            spawn_count.y += 60;

            text(Widget{spawn_count}, fmt::format("Balance: {}", balance));
        }

        {
            Rectangle spawn_count(left_col);
            spawn_count.y += 120;

            text(Widget{spawn_count}, fmt::format("In Cart: {}", cart),
                 cart <= balance ? ui::theme::Usage::Font
                                 : ui::theme::Usage::Error);
        }
    }

    void render_validation(Rectangle content) {
        using namespace ui;
        OptEntity purchase_area = EntityHelper::getMatchingTriggerArea(
            IsTriggerArea::Type::Store_BackToPlanning);

        if (!purchase_area.valid()) return;

        const IsTriggerArea& ita = purchase_area->get<IsTriggerArea>();
        if (ita.should_progress()) return;

        // TODO this is where the translation should happen
        text(Widget{content}, fmt::format("{}", ita.validation_msg()));
    }

    virtual void onDrawUI(float) override {
        using namespace ui;

        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        auto left_col = rect::tpad(window, 10);
        left_col = rect::rpad(left_col, 20);
        left_col = rect::lpad(left_col, 10);
        left_col = rect::bpad(left_col, 20);

        render_balances(left_col);

        auto content = rect::tpad(window, 30);
        content = rect::lpad(content, 20);
        content = rect::rpad(content, 75);
        content = rect::bpad(content, 80);

        render_validation(content);
    }
};
