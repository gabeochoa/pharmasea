
#pragma once

#include "../engine.h"
#include "../entity.h"
#include "../external_include.h"
#include "../preload.h"

struct PlayerInfoLayer : public Layer {
    PlayerInfoLayer() : Layer("PlayerInfo") {}

    int y_pos = 0;

    virtual ~PlayerInfoLayer() {}
    virtual void onUpdate(float) override {}

    void draw_text(const std::string& str) {
        float y = 200.f + y_pos;
        DrawTextEx(Preload::get().font, str.c_str(), vec2{5, y}, 20, 0, WHITE);
        y_pos += 15;
    }

    virtual void onDraw(float) override {
        if (!MenuState::s_in_game()) return;

        Entity* me = GLOBALS.get_ptr<Entity>("active_camera_target");
        if (!me) return;

        DrawRectangle(5, 200, 175, 75, (Color){50, 50, 50, 200});

        y_pos = 0;
        draw_text("PlayerInfo:");
        draw_text(fmt::format("position: {}", me->get<Transform>().pos()));
        draw_text(
            fmt::format("holding furniture?: {}",
                        me->get<CanHoldFurniture>().is_holding_furniture()));
        draw_text(fmt::format("holding item?: {}",
                              me->get<CanHoldItem>().is_holding_item()));
    }
};
