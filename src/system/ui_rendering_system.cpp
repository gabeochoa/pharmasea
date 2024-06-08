
#include "ui_rendering_system.h"

#include "../components/can_order_drink.h"
#include "../components/has_client_id.h"
#include "../components/has_name.h"
#include "../components/has_patience.h"
#include "../components/has_speech_bubble.h"
#include "../components/has_timer.h"
#include "../components/has_waiting_queue.h"
#include "../components/is_bank.h"
#include "../components/model_renderer.h"
#include "../entity_helper.h"
#include "../entity_query.h"

namespace system_manager {
namespace ui {

struct OrderCard {
    raylib::Texture icon;
    float pct_time_left;
    int z_index;
};
static std::vector<OrderCard> orders_to_render;
static float refresh_time = 0.f;

void render_current_register_queue(float dt) {
    refresh_time -= dt;
    if (refresh_time < 0) {
        refresh_time = 2.f;

        const std::vector<RefEntity> registers =
            EntityQuery().whereType(EntityType::Register).gen();

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
                        entity.name());
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
        raylib::DrawRectangleRounded(
            card_index_rect, 0.5f, 8,
            ::ui::get_default_progress_bar_color(card.pct_time_left));

        float scale = 2.f;
        raylib::DrawTextureEx(card.icon, {card_index_rect.x, card_index_rect.y},
                              0, scale, WHITE);
    }
}

void render_timer(const Entity& entity, float) {
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

void render_networked_players(const Entities& entities, float dt) {
    float x_pos = WIN_WF() - 170;
    float y_pos = 75.f;

    auto _draw_text = [&](const std::string& str) mutable {
        int size = 20;
        DrawTextEx(Preload::get().font, str.c_str(), vec2{x_pos, y_pos}, size,
                   0, WHITE);
        y_pos += (size * 1.25f);
    };

    const auto _render_single_networked_player = [&](const Entity& entity,
                                                     float) {
        _draw_text(                                    //
            fmt::format("{}({}) {}",                   //
                        entity.get<HasName>().name(),  //
                        entity.get<HasClientID>().id(),
                        // TODO replace with icon
                        entity.get<HasClientID>().ping()));
    };
    const auto _render_little_model_guy = [&](const Entity& entity, float) {
        if (entity.is_missing<ModelRenderer>()) {
            log_warn(
                "render_little_model_guy, entity {} is missing model renderer",
                entity.name());
            return;
        }
        auto model_name = entity.get<ModelRenderer>().name();
        raylib::Texture texture =
            TextureLibrary::get().get(fmt::format("{}_mug", model_name));
        float scale = 0.06f;
        raylib::DrawTextureEx(texture,
                              {x_pos - (500 /* image size */ * scale), y_pos},
                              0, scale, WHITE);
    };

    for (const auto& entity_ptr : entities) {
        if (!entity_ptr) continue;
        Entity& entity = *entity_ptr;
        // TODO think about this check more
        if (!(check_type(entity, EntityType::Player) ||
              check_type(entity, EntityType::RemotePlayer)))
            continue;
        _render_little_model_guy(entity, dt);
        _render_single_networked_player(entity, dt);
    }
}

struct RoundTimerLocation {
    vec2 center;
    float radius;
    float r10;
    vec2 start;
    vec2 end;
    Rectangle rounded_rect;
};

RoundTimerLocation get_round_timer_location() {
    auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
    window = rect::tpad(window, 10);
    window = rect::rpad(window, 20);
    window = rect::lpad(window, 10);
    window = rect::bpad(window, 20);

    const auto center = vec2{
        window.x + (window.width / 2.f),
        window.y + (window.height / 2.f),
    };

    const float radius = WIN_WF() / 64.f;
    const float r10 = radius * 6;
    const vec2 start = {center.x - r10 - (radius), center.y};
    const vec2 end = {center.x + r10 + (radius), center.y};

    Rectangle rounded_rect = {start.x,          //
                              start.y - 10,     //
                              end.x - start.x,  //
                              radius * 2.f};

    return RoundTimerLocation{
        .center = center,
        .radius = radius,
        .r10 = r10,
        .rounded_rect = rounded_rect,
    };
}

TranslatableString get_status_text(const HasTimer& ht) {
    const bool is_closing = ht.store_is_closed();
    const bool is_day = GameState::get().in_round() && !is_closing;
    const int dayCount = ht.dayCount;

    auto status_text =
        is_day ? TranslatableString(strings::i18n::OPEN)
               : (is_closing ? TranslatableString(strings::i18n::CLOSING)
                             : TranslatableString(strings::i18n::CLOSED));

    return TranslatableString(strings::i18n::RoundDayWithStatusText)
        .set_param(strings::i18nParam::OpeningStatus, status_text)
        .set_param(strings::i18nParam::DayCount, dayCount);
}

void render_round_timer(const Entity& entity, float) {
    if (entity.is_missing<HasTimer>()) return;

    const HasTimer& ht = entity.get<HasTimer>();
    if (ht.type != HasTimer::Renderer::Round) return;

    const auto rtl = get_round_timer_location();

    const bool is_closing = ht.store_is_closed();
    const bool is_day = GameState::get().in_round() && !is_closing;
    const float pct = ht.pct();
    const float angle = util::deg2rad(util::lerp(170, 365, 1 - pct));

    const vec2 pos = {
        rtl.center.x + std::cos(angle) * (rtl.r10),
        rtl.center.y + std::sin(angle) * (rtl.r10),
    };

    // Hide it when its below the rect
    if (angle >= M_PI)
        raylib::DrawCircle((int) pos.x, (int) pos.y, rtl.radius,
                           is_day ? YELLOW : GRAY);

    div(::ui::Widget{rtl.rounded_rect},
        is_day ? ::ui::theme::Primary : ::ui::theme::Background);
    text(::ui::Widget{rtl.rounded_rect}, get_status_text(ht));

    // Only show the customer count during planning
    if (GameState::get().is(game::State::Planning)) {
        Rectangle spawn_count{rtl.rounded_rect};
        spawn_count.y += 160;

        Entity& spawner =
            (EntityQuery().whereType(EntityType::CustomerSpawner).gen_first())
                .asE();
        const IsSpawner& iss = spawner.get<IsSpawner>();
        text(::ui::Widget{spawn_count},
             TranslatableString(strings::i18n::PLANNING_CUSTOMERS_COMING)
                 .set_param(strings::i18nParam::CustomerCount,
                            iss.get_max_spawned()));
    }
}

void render_animated_transactions(const Entity& entity, float) {
    if (!check_type(entity, EntityType::Sophie)) return;

    Rectangle spawn_count(get_round_timer_location().rounded_rect);
    spawn_count.y += 80;

    const IsBank& bank = entity.get<IsBank>();

    text(::ui::Widget{spawn_count},
         TranslatableString(strings::i18n::StoreBalance)
             .set_param(strings::i18nParam::BalanceAmount, bank.balance()));

    const std::vector<IsBank::Transaction>& transactions =
        bank.get_transactions();
    if (!transactions.empty()) {
        const IsBank::Transaction& transaction = transactions.front();
        const auto animate_new_transaction =
            [](const IsBank::Transaction& transaction, Rectangle spawn_count) {
                if (transaction.amount == 0 && transaction.extra == 0) return;

                spawn_count.y += static_cast<int>(60 * transaction.pct());

                bool positive = transaction.amount >= 0;
                unsigned char alpha =
                    static_cast<unsigned char>(255 * transaction.pct());

                auto tip_string = translation_lookup(
                    TranslatableString(strings::i18n::StoreTip)
                        .set_param(strings::i18nParam::TransactionExtra,
                                   transaction.extra));

                colored_text(
                    ::ui::Widget{spawn_count},
                    TODO_TRANSLATE(
                        fmt::format("          {}{} {}",
                                    positive ? "+" : "-",  //
                                    transaction.amount,
                                    transaction.extra ? tip_string : ""),
                        TodoReason::Format),
                    positive ? Color{0, 255, 0, alpha} : Color{255, 0, 0, alpha}
                    //
                );
            };

        animate_new_transaction(transaction, spawn_count);
    }
}

void render_normal(const Entities& entities, float dt) {
    // In game only
    if (GameState::get().should_render_timer()) {
        // Grab the global ui context,
        ::ui::begin(::ui::context, dt);

        for (const auto& entity_ptr : entities) {
            if (!entity_ptr) continue;
            const Entity& entity = *entity_ptr;
            render_timer(entity, dt);
            render_animated_transactions(entity, dt);
            render_round_timer(entity, dt);
        }
        render_current_register_queue(dt);
        //
        ::ui::end();
    }

    // always
    render_networked_players(entities, dt);
}

}  // namespace ui
}  // namespace system_manager
