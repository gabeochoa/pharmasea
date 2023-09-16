

#include "../components/can_order_drink.h"
#include "../components/has_patience.h"
#include "../components/has_speech_bubble.h"
#include "../components/has_timer.h"
#include "../components/has_waiting_queue.h"
#include "../engine/ui/ui.h"
#include "../entity.h"
#include "../entity_helper.h"

namespace system_manager {
namespace ui {

struct OrderCard {
    raylib::Texture icon;
    float pct_time_left;
    int z_index;
};
static std::vector<OrderCard> orders_to_render;
static float refresh_time = 0.f;

inline void render_current_register_queue(float dt) {
    refresh_time -= dt;
    if (refresh_time < 0) {
        refresh_time = 2.f;

        const std::vector<RefEntity> registers =
            EntityHelper::getAllWithType(EntityType::Register);

        orders_to_render.clear();

        for (size_t i = 0; i < registers.size(); i++) {
            Entity& reg = registers[i];
            const HasWaitingQueue& hwq = reg.get<HasWaitingQueue>();

            for (int index = 0; index < hwq.num_in_queue(); index++) {
                EntityID e_id = hwq.person(index);
                OptEntity opt_e = EntityHelper::getEntityForID(e_id);
                if (!opt_e) {
                    // log_warn("register {} has null people in line", i);
                    continue;
                }
                Entity& entity = opt_e.asE();
                if (entity.is_missing<CanOrderDrink>()) {
                    log_warn(
                        "register {} has people who cant order in line: {}", i,
                        entity.get<DebugName>());
                    continue;
                }

                // Hide any that arent "visible"
                if (entity.get<HasSpeechBubble>().disabled()) continue;
                // if (!entity.get<HasPatience>().should_pass_time()) continue;

                raylib::Texture texture =  //
                    TextureLibrary::get().get(
                        entity.get<CanOrderDrink>().icon_name());

                orders_to_render.emplace_back(OrderCard{
                    texture, entity.get<HasPatience>().pct(), (int) index});
            }
        }
    }

    int num_queue = (int) orders_to_render.size();
    int max_num_cards = 10;
    bool show_see_more = (max_num_cards - num_queue) < 0;

    // Render stuff

    float height = 0.1f * WIN_HF();
    float width = height * max_num_cards;
    Rectangle queue_window = {
        0.2f * WIN_WF(),  //
        0,                //
        width,
        height,  //
    };

    Rectangle card_rect = {0,       //
                           0,       //
                           height,  //
                           height};

    // Disable queue window for now because we dont need to see it
    // const auto debug_mode_on =
    // GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
    // if (debug_mode_on) {
    // raylib::DrawRectangleRounded(
    // queue_window, 0.5f, 8,
    // ::ui::UI_THEME.from_usage(::ui::theme::Background));
    // }

    // TODO :WARNING_COLORS: Eventually merge this with the one in
    // rendering_system
    const auto _pct_to_color = [](float pct) {
        if (pct < 0.05f)
            return ::ui::UI_THEME.from_usage(::ui::theme::Usage::Error);
        if (pct < 0.2f)
            return ::ui::UI_THEME.from_usage(::ui::theme::Usage::Accent);
        return ::ui::UI_THEME.from_usage(::ui::theme::Usage::Primary);
    };

    for (int i = 0; i < fmin(num_queue, max_num_cards); i++) {
        if (show_see_more && i == (max_num_cards - 1)) {
            // TODO render see more
            // only will show if you have num_registers > max_num_cards
            continue;
        }

        auto card = orders_to_render[i];

        auto card_index_rect =
            Rectangle{queue_window.x + (card_rect.width * i), queue_window.y,
                      card_rect.width, card_rect.height};
        raylib::DrawRectangleRounded(card_index_rect, 0.5f, 8,
                                     _pct_to_color(card.pct_time_left));

        float scale = 2.f;
        raylib::DrawTextureEx(card.icon, {card_index_rect.x, card_index_rect.y},
                              0, scale, WHITE);
    }
}

inline void render_timer(const Entity& entity, float) {
    if (entity.is_missing<HasTimer>()) return;

    auto& ht = entity.get<HasTimer>();
    switch (ht.type) {
        case HasTimer::Renderer::Round: {
            // Rendered by round_timer_layer.h
        } break;
        default:
            log_warn("you have a timer that we dont know how to render: {}",
                     ht.type);
            break;
    }
}

void render_networked_players(const Entities&, float dt);

inline void render_normal(const Entities& entities, float dt) {
    // In game only
    if (GameState::get().should_render_timer()) {
        for (const auto& entity_ptr : entities) {
            if (!entity_ptr) continue;
            const Entity& entity = *entity_ptr;
            render_timer(entity, dt);
        }
    }
    // TODO move into render timer check
    render_current_register_queue(dt);

    // always
    render_networked_players(entities, dt);
}

}  // namespace ui
}  // namespace system_manager
