#pragma once

#include "../components/has_timer.h"
#include "../components/is_bank.h"
#include "../components/is_spawner.h"
#include "../entity.h"
#include "../entity_helper.h"
#include "base_game_renderer.h"

struct StoreLayer : public BaseGameRendererLayer {
    float _pct = 0.f;

    StoreLayer() : BaseGameRendererLayer("Store") {}

    virtual ~StoreLayer() {}

    virtual bool shouldSkipRender() override {
        if (!GameState::get().is(game::State::Store)) return true;
        return false;
    }

    virtual void onDrawUI(float) override {
        using namespace ui;

        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        window = rect::tpad(window, 10);
        window = rect::rpad(window, 20);
        window = rect::lpad(window, 10);
        window = rect::bpad(window, 20);

        OptEntity sophie = EntityHelper::getFirstOfType(EntityType::Sophie);
        if (!sophie.valid()) return;
        const IsBank& bank = sophie->get<IsBank>();
        int balance = bank.balance();
        int cart = bank.cart();

        {
            Rectangle spawn_count(window);
            spawn_count.y += 60;

            text(Widget{spawn_count}, fmt::format("Balance: {}", balance));
        }

        {
            Rectangle spawn_count(window);
            spawn_count.y += 120;

            text(Widget{spawn_count}, fmt::format("In Cart: {}", cart),
                 cart <= balance ? ui::theme::Usage::Font
                                 : ui::theme::Usage::Error);
        }
    }
};
