

#include "../components/can_order_drink.h"
#include "../components/has_speech_bubble.h"
#include "../components/has_timer.h"
#include "../components/has_waiting_queue.h"
#include "../engine/ui.h"
#include "../entity.h"
#include "../entity_helper.h"

namespace system_manager {
namespace ui {

struct OrderCard {
    raylib::Texture icon;
    float time_left;
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

            for (size_t index = 0; index < hwq.num_in_queue(); index++) {
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

                raylib::Texture texture =  //
                    TextureLibrary::get().get(
                        entity.get<CanOrderDrink>().icon_name());

                orders_to_render.emplace_back(OrderCard{texture,
                                                        // TODO add patience
                                                        10.f, (int) index});
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
        height,           //
    };

    Rectangle card_rect = {0,       //
                           0,       //
                           height,  //
                           height};

    const auto debug_mode_on =
        GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
    if (debug_mode_on) {
        raylib::DrawRectangleRounded(
            queue_window, 0.5f, 8,
            ::ui::DEFAULT_THEME.from_usage(::ui::theme::Background));
    }

    for (int i = 0; i < fmin(num_queue, max_num_cards); i++) {
        if (show_see_more && i == (max_num_cards - 1)) {
            // TODO render see more
            // only will show if you have num_registers > max_num_cards
            continue;
        }

        auto card_index_rect =
            Rectangle{queue_window.x + (card_rect.width * i), queue_window.y,
                      card_rect.width, card_rect.height};
        raylib::DrawRectangleRounded(
            card_index_rect, 0.5f, 8,
            ::ui::DEFAULT_THEME.from_usage(::ui::theme::Primary));

        auto card = orders_to_render[i];

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
            const bool is_closing = ht.store_is_closed();
            const bool is_day = GameState::get().in_round() && !is_closing;

            const vec2 center = {200.f, 75.f};

            const float radius = 10;
            const float r5 = radius * 5;
            const float angle =
                util::deg2rad(util::lerp(175, 360, 1 - ht.pct()));
            const vec2 pos = {
                center.x + std::cos(angle) * (r5),
                center.y + std::sin(angle) * (r5),
            };

            // Hide it when its below the rect
            if (angle >= M_PI)
                raylib::DrawCircle((int) pos.x, (int) pos.y, radius,
                                   is_day ? YELLOW : GRAY);

            const vec2 start = {center.x - r5 - (radius), center.y};
            const vec2 end = {center.x + r5 + (radius), center.y};
            vec2 rect_size = {end.x - start.x, 20};
            vec2 rect_pos = {start.x, start.y - 10};

            // TODO change to varargs with structured bindings?
            // or colocate with usage
            Color bg = ::ui::DEFAULT_THEME.from_usage(::ui::theme::Background);
            Color primary =
                ::ui::DEFAULT_THEME.from_usage(::ui::theme::Primary);
            Color font_color =
                ::ui::DEFAULT_THEME.from_usage(::ui::theme::Font);

            raylib::DrawRectangleRounded(
                {rect_pos.x, rect_pos.y, rect_size.x, rect_size.y}, 0.5f, 8,
                is_day ? primary : bg);

            auto status_text =
                is_day ? text_lookup(strings::i18n::OPEN)
                       : (is_closing ? text_lookup(strings::i18n::CLOSING)
                                     : text_lookup(strings::i18n::CLOSED));

            // TODO figure out how to translate strings that have numbers in
            // them...
            auto day_text = fmt::format("{} {}", "Day", ht.dayCount);

            raylib::DrawTextEx(
                Preload::get().font,
                fmt::format("{} {}", status_text, day_text).c_str(),
                {rect_pos.x, rect_pos.y - 2}, 20, 0, font_color);

        } break;
        default:
            log_warn("you have a timer that we dont know how to render: {}",
                     ht.type);
            break;
    }
}

inline void render_block_state_change_reason(const Entity& entity, float) {
    if (entity.is_missing<HasTimer>()) return;
    const auto& ht = entity.get<HasTimer>();

    const auto debug_mode_on =
        GLOBALS.get_or_default<bool>("debug_ui_enabled", false);

    // if the round isnt over dont need to show anything
    if (ht.currentRoundTime > 0 && !debug_mode_on) return;

    //
    auto _render_single_reason = [](const std::string& text, float y) {
        Color font_color = ::ui::DEFAULT_THEME.from_usage(::ui::theme::Font);
        raylib::DrawTextEx(Preload::get().font, text.c_str(), {200, y}, 75, 0,
                           font_color);
    };

    // TODO handle centering the text better when there are more than one
    bool has_reasons = ht.block_state_change_reasons.count() > 1;
    int posy = has_reasons ? 25 : 25;
    for (int i = 1; i < HasTimer::WaitingReason::WaitingReasonLast; i++) {
        bool enabled = ht.read_reason(i);

        if (enabled) {
            _render_single_reason(ht.text_reason(i), posy);
            posy += 75;
        }
    }

    if (ht.block_state_change_reasons.none()) {
        Color font_color = ::ui::DEFAULT_THEME.from_usage(::ui::theme::Font);
        auto countdown = fmt::format(
            "{}: {}", text_lookup(strings::i18n::NEXT_ROUND_COUNTDOWN),
            (int) ceil(util::trunc(ht.roundSwitchCountdown, 1)));
        raylib::DrawTextEx(Preload::get().font, countdown.c_str(), {200, 100},
                           75, 0, font_color);
    }
}

void render_player_info(const Entity& entity);

void render_networked_players(const Entities&, float dt);

inline void render_debug_ui(const Entities& entities, float) {
    for (std::shared_ptr<Entity> entity_ptr : entities) {
        if (!entity_ptr) continue;
        const Entity& entity = *entity_ptr;
        render_player_info(entity);
    }
}

inline void render_current_register_queue(float dt);

inline void render_normal(const Entities& entities, float dt) {
    // In game only
    if (GameState::get().should_render_timer()) {
        for (const auto& entity_ptr : entities) {
            if (!entity_ptr) continue;
            const Entity& entity = *entity_ptr;
            render_timer(entity, dt);
            render_block_state_change_reason(entity, dt);
        }
    }
    // TODO move into render timer check
    render_current_register_queue(dt);

    // always
    render_networked_players(entities, dt);
}

}  // namespace ui
}  // namespace system_manager
