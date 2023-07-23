

#pragma once

#include "../camera.h"
#include "../components/uses_character_model.h"
#include "../engine/log.h"
#include "../engine/time.h"
#include "job_system.h"

namespace system_manager {
namespace render_manager {

inline void draw_valid_colored_box(const Transform& transform,
                                   const SimpleColoredBoxRenderer& renderer,
                                   bool is_highlighted) {
    Color f = renderer.face();
    // TODO Maybe should move into DrawCubeCustom
    if (ui::color::is_empty(f)) {
        log_warn("Face color is empty");
        f = PINK;
    }

    Color b = renderer.base();
    if (ui::color::is_empty(b)) {
        log_warn("Base color is empty");
        b = PINK;
    }

    if (is_highlighted) {
        f = ui::color::getHighlighted(f);
        b = ui::color::getHighlighted(b);
    }

    DrawCubeCustom(
        transform.raw(), transform.sizex(), transform.sizey(),
        transform.sizez(),
        transform.FrontFaceDirectionMap.at(transform.face_direction()), f, b);
}

inline void update_character_model_from_index(std::shared_ptr<Entity> entity,
                                              float) {
    if (!entity) return;

    if (entity->is_missing<UsesCharacterModel>()) return;
    UsesCharacterModel& usesCharacterModel = entity->get<UsesCharacterModel>();

    if (usesCharacterModel.value_same_as_last_render()) return;

    if (entity->is_missing<ModelRenderer>()) return;

    ModelRenderer& renderer = entity->get<ModelRenderer>();

    // TODO this should be the same as all other rendere updates for players
    renderer.update_model_name(usesCharacterModel.fetch_model_name());
    usesCharacterModel.mark_change_completed();
}

inline bool render_simple_highlighted(const Entity& entity, float) {
    if (entity.is_missing<Transform>()) return false;
    const Transform& transform = entity.get<Transform>();
    if (entity.is_missing<SimpleColoredBoxRenderer>()) return false;
    const SimpleColoredBoxRenderer& renderer =
        entity.get<SimpleColoredBoxRenderer>();
    // TODO replace size with Bounds component when it exists
    draw_valid_colored_box(transform, renderer, true);
    return true;
}

inline bool render_simple_normal(const Entity& entity, float) {
    if (entity.is_missing<Transform>()) return false;
    const Transform& transform = entity.get<Transform>();
    if (entity.is_missing<SimpleColoredBoxRenderer>()) return false;
    const SimpleColoredBoxRenderer& renderer =
        entity.get<SimpleColoredBoxRenderer>();

    draw_valid_colored_box(transform, renderer, false);
    return true;
}

inline bool render_bounding_box(const Entity& entity, float) {
    if (entity.is_missing<Transform>()) return false;
    const Transform& transform = entity.get<Transform>();

    DrawBoundingBox(transform.bounds(), MAROON);
    DrawFloatingText(transform.raw(), Preload::get().font,
                     fmt::format("{}", entity.id).c_str());
    return true;
}

inline void render_debug_subtype(const Entity& entity, float) {
    if (entity.is_missing<HasSubtype>()) return;
    const HasSubtype& hs = entity.get<HasSubtype>();
    const Transform& transform = entity.get<Transform>();

    std::string content = fmt::format("{}", hs.get_type_index());

    // Convert from ID to ingredient if its an alcohol
    if (check_name(entity, strings::item::ALCOHOL)) {
        content = fmt::format("{}", hs.as_type<Ingredient>());
    }

    DrawFloatingText(vec::raise(transform.raw(), 1.f), Preload::get().font,
                     content.c_str());
}

inline void render_debug_drink_info(const Entity& entity, float) {
    if (entity.is_missing<IsDrink>()) return;
    const IsDrink& isdrink = entity.get<IsDrink>();
    const Transform& transform = entity.get<Transform>();
    float y = 0.25f;
    for (size_t i = 0; i < magic_enum::enum_count<Ingredient>(); i++) {
        Ingredient ingredient = magic_enum::enum_value<Ingredient>(i);
        if (!(isdrink.has_ingredient(ingredient))) continue;
        auto content = magic_enum::enum_name(ingredient);
        DrawFloatingText(vec::raise(transform.raw(), y), Preload::get().font,
                         std::string(content).c_str(), 50);
        y += 0.25f;
    }
}

inline void render_debug_filter_info(const Entity& entity, float) {
    if (entity.is_missing<CanHoldItem>()) return;
    const EntityFilter& filter = entity.get<CanHoldItem>().get_filter();
    if (filter.flags == 0) return;
    if (!filter.filter_is_set()) return;

    const Transform& transform = entity.get<Transform>();

    float y = 0.25f;
    for (size_t i = 1; i < filter.type_count(); i++) {
        EntityFilter::FilterDatumType type =
            magic_enum::enum_value<EntityFilter::FilterDatumType>(i);

        auto filter_name = magic_enum::enum_name(type);
        auto filter_value = filter.print_value_for_type(type);

        auto content = fmt::format("filter {}: {}", filter_name, filter_value);

        DrawFloatingText(vec::raise(transform.raw(), y), Preload::get().font,
                         std::string(content).c_str(), 50);
        y += 0.25f;
    }
}

inline bool render_debug(const Entity& entity, float dt) {
    job_system::render_job_visual(entity, dt);

    render_debug_subtype(entity, dt);
    render_debug_drink_info(entity, dt);
    render_debug_filter_info(entity, dt);

    // Ghost player only render during debug mode
    if (entity.has<CanBeGhostPlayer>() &&
        entity.get<CanBeGhostPlayer>().is_ghost()) {
        render_simple_normal(entity, dt);
    }
    return render_bounding_box(entity, dt);
}

inline bool render_model_highlighted(const Entity& entity, float) {
    if (entity.is_missing<ModelRenderer>()) return false;
    if (entity.is_missing<CanBeHighlighted>()) return false;
    if (entity.is_missing<Transform>()) return false;
    const Transform& transform = entity.get<Transform>();
    const ModelRenderer& renderer = entity.get<ModelRenderer>();
    if (renderer.missing()) return false;
    ModelInfo& model_info = renderer.model_info();

    // TODO this is the exact same code as render_model_normal
    // should be able to fix it

    float rotation_angle =
        // TODO make this api better
        180.f + static_cast<int>(transform.FrontFaceDirectionMap.at(
                    transform.face_direction()));

    DrawModelEx(renderer.model(),
                {
                    transform.pos().x + model_info.position_offset.x,
                    transform.pos().y + model_info.position_offset.y,
                    transform.pos().z + model_info.position_offset.z,
                },
                vec3{0.f, 1.f, 0.f}, model_info.rotation_angle + rotation_angle,
                transform.size() * model_info.size_scale, GRAY);

    return true;
}

inline bool render_model_normal(const Entity& entity, float) {
    if (!ENABLE_MODELS) return false;
    if (entity.is_missing<ModelRenderer>()) return false;
    if (entity.is_missing<Transform>()) return false;
    const Transform& transform = entity.get<Transform>();

    const ModelRenderer& renderer = entity.get<ModelRenderer>();
    if (renderer.missing()) return false;

    const ModelInfo& model_info = renderer.model_info();

    float rotation_angle =
        // TODO make this api better
        180.f + static_cast<int>(transform.FrontFaceDirectionMap.at(
                    transform.face_direction()));

    raylib::DrawModelEx(
        renderer.model(),
        {
            transform.pos().x + model_info.position_offset.x,
            transform.pos().y + model_info.position_offset.y,
            transform.pos().z + model_info.position_offset.z,
        },
        vec3{0, 1, 0}, model_info.rotation_angle + rotation_angle,
        transform.size() * model_info.size_scale, WHITE /*this->base_color*/);

    return true;
}

inline void render_trigger_area(const Entity& entity, float dt) {
    if (entity.is_missing<IsTriggerArea>()) return;

    // TODO add highlight when you walk in
    // TODO add progress bar when all players are inside

    const IsTriggerArea& ita = entity.get<IsTriggerArea>();

    const Transform& transform = entity.get<Transform>();

    vec3 pos = transform.pos();
    vec3 size = transform.size();

    vec3 text_position = {pos.x - (size.x / 2.f),     //
                          pos.y + (TILESIZE / 10.f),  //
                          pos.z - (size.z / 3.f)};

    vec3 number_position = {pos.x + (size.x / 3.f),
                            pos.y + (TILESIZE / 5.f),  //
                            pos.z + (size.z / 4.f)};
    // Top left text position
    // {pos.x - (size.x / 2.f),     //
    // pos.y + (TILESIZE / 20.f),  //
    // pos.z - (size.z / 2.f)};

    auto font = Preload::get().font;
    // TODO eventually we can detect the size to fit the text correctly
    // but thats more of an issue for translations since i can manually
    // place the english
    auto fsize = 500.f;

    std::string title = entity.get<IsTriggerArea>().title();
    log_ifx(title.empty(), LogLevel::LOG_WARN,
            "Rendering trigger area with empty text string: id{} pos{}",
            entity.id, pos);

    if (ita.should_wave()) {
        raylib::WaveTextConfig waveConfig = {.waveRange = {0, 0, TILESIZE},
                                             .waveSpeed = {0, 0, 0.01f},
                                             .waveOffset = {0.f, 0.f, 0.2f}};
        // TODO we probably should have all of this kind of thing in the config
        // for the components
        //
        raylib::DrawTextWave3D(
            font,
            fmt::format("~~{}~~", text_lookup(strings::i18n::LOADING)).c_str(),
            text_position, fsize,
            4,                  // font spacing
            4,                  // line spacing
            false,              // backface
            &waveConfig,        //
            now::current_ms(),  //
            WHITE);

    } else {
        raylib::DrawText3D(font, title.c_str(), text_position, fsize,
                           4,      // font spacing
                           4,      // line spacing
                           false,  // backface
                           WHITE);

        raylib::DrawText3D(
            font,
            fmt::format("{}/{}", ita.active_entrants(), ita.max_entrants())
                .c_str(),
            number_position,  //
            fsize / 2.f,
            4,                // font spacing
            4,                // line spacing
            false,            // backface
            WHITE);
    }

    if (ita.progress() > 0.f) {
        // TODO switch to using a validated draw_cube
        DrawCubeCustom(
            {
                (pos.x + ((size.x * ita.progress()) / 2.f)) -
                    (size.x / 2.f),         //
                pos.y + (TILESIZE / 20.f),  //
                pos.z                       //
            },
            size.x * ita.progress(),        //
            size.y,                         //
            size.z,
            transform.FrontFaceDirectionMap.at(transform.face_direction()), RED,
            RED);
    }

    render_simple_normal(entity, dt);
}

inline void render_speech_bubble(const Entity& entity, float) {
    // Right now this is the only thing we can put in a bubble
    if (entity.is_missing<CanOrderDrink>()) return;
    if (entity.get<HasSpeechBubble>().disabled()) return;

    const Transform& transform = entity.get<Transform>();
    const vec3 position = transform.pos();

    const CanOrderDrink& cod = entity.get<CanOrderDrink>();

    GameCam cam = GLOBALS.get<GameCam>(strings::globals::GAME_CAM);
    raylib::Texture texture = TextureLibrary::get().get(cod.icon_name());
    raylib::DrawBillboard(cam.camera, texture,
                          vec3{position.x,                     //
                               position.y + (TILESIZE * 2.f),  //
                               position.z},                    //
                          TILESIZE,                            //
                          raylib::WHITE);
}

// TODO theres two functions called render normal, maybe we should address this
inline void render_normal(const Entity& entity, float dt) {
    //  TODO for now while we do dev work render it
    render_debug_subtype(entity, dt);
    render_debug_drink_info(entity, dt);
    render_debug_filter_info(entity, dt);

    // Ghost player cant render during normal mode
    if (entity.has<CanBeGhostPlayer>() &&
        entity.get<CanBeGhostPlayer>().is_ghost()) {
        return;
    }

    if (entity.has<IsTriggerArea>()) {
        render_trigger_area(entity, dt);
        return;
    }

    if (entity.has<CanBeHighlighted>() &&
        entity.get<CanBeHighlighted>().is_highlighted()) {
        bool used = render_model_highlighted(entity, dt);
        if (!used) {
            render_simple_highlighted(entity, dt);
        }
        return;
    }

    if (entity.has<HasSpeechBubble>()) {
        render_speech_bubble(entity, dt);
    }

    bool used = render_model_normal(entity, dt);
    if (!used) {
        render_simple_normal(entity, dt);
    }
}

inline void render_floating_name(const Entity& entity, float) {
    if (entity.is_missing<HasName>()) return;
    const HasName& hasName = entity.get<HasName>();

    if (entity.is_missing<Transform>()) return;
    const Transform& transform = entity.get<Transform>();

    // log_warn("drawing floating name {} for {} @ {} ({})", hasName.name(),
    // entity.id, transform.position, transform.raw_position);

    // TODO rotate the name with the camera?
    raylib::DrawFloatingText(
        transform.raw() + vec3{0, 0.5f * TILESIZE, 0.2f * TILESIZE},
        Preload::get().font, hasName.name().c_str());
}

inline void render_progress_bar(const Entity& entity, float) {
    if (entity.is_missing<ShowsProgressBar>()) return;

    if (entity.is_missing<Transform>()) return;
    const Transform& transform = entity.get<Transform>();

    if (entity.is_missing<HasWork>()) return;

    const HasWork& hasWork = entity.get<HasWork>();
    if (hasWork.dont_show_progres_bar()) return;

    const int length = 20;
    const int full = hasWork.scale_length(length);
    const int empty = length - full;
    DrawFloatingText(                                                     //
        transform.raw() + vec3({0, TILESIZE, 0}),                         //
        Preload::get().font,                                              //
        fmt::format("[{:=>{}}{: >{}}]", "", full, "", empty * 2).c_str()  //
    );

    // TODO eventually add real rectangle progress bar
    // auto game_cam = GLOBALS.get<GameCam>("game_cam");
    // DrawBillboardRec(game_cam.camera, TextureLibrary::get().get("face"),
    // Rectangle({0, 0, 260, 260}),
    // this->raw_position + vec3({-(pct_complete * TILESIZE),
    // TILESIZE * 2, 0}),
    // {pct_complete * TILESIZE, TILESIZE}, WHITE);
}

}  // namespace render_manager

namespace ui {

inline void render_timer(const Entity& entity, float) {
    if (entity.is_missing<HasTimer>()) return;

    auto& ht = entity.get<HasTimer>();
    switch (ht.type) {
        case HasTimer::Renderer::Round: {
            const bool is_closing = ht.store_is_closed();
            const bool is_day = GameState::s_in_round() && !is_closing;

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

            raylib::DrawTextEx(Preload::get().font, status_text,
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
    auto _render_single_reason = [](std::string text, float y) {
        Color font_color = ::ui::DEFAULT_THEME.from_usage(::ui::theme::Font);
        raylib::DrawTextEx(Preload::get().font, text.c_str(), {200, y}, 75, 0,
                           font_color);
    };

    // TODO handle centering the text better when there are more than one
    bool has_reasons = ht.block_state_change_reasons.count() > 1;
    int posy = has_reasons ? 75 : 200;
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

inline void render_player_info(const Entity& entity) {
    if (!check_name(entity, strings::entity::REMOTE_PLAYER)) return;

    if (entity.id != global_player->id) return;

    int y_pos = 0;

    auto _draw_text = [&](const std::string& str) mutable {
        float y = 200.f + y_pos;
        DrawTextEx(Preload::get().font, str.c_str(), vec2{5, y}, 20, 0, WHITE);
        y_pos += 15;
    };

    raylib::DrawRectangle(5, 200, 175, 75, (Color){50, 50, 50, 200});

    _draw_text("PlayerInfo:");
    _draw_text(fmt::format("id: {} position: {}", entity.id,
                           entity.get<Transform>().pos()));
    _draw_text(
        fmt::format("holding furniture?: {}",
                    entity.get<CanHoldFurniture>().is_holding_furniture()));
    _draw_text(fmt::format("holding item?: {}",
                           entity.get<CanHoldItem>().is_holding_item()));
}

void render_networked_players(const Entities&, float dt);

inline void render_debug_ui(const Entities& entities, float) {
    for (std::shared_ptr<Entity> entity_ptr : entities) {
        const Entity& entity = *entity_ptr;
        render_player_info(entity);
    }
}

inline void render_normal(const Entities& entities, float dt) {
    for (auto& entity_ptr : entities) {
        const Entity& entity = *entity_ptr;
        render_timer(entity, dt);
        render_block_state_change_reason(entity, dt);
    }
    render_networked_players(entities, dt);
}

}  // namespace ui

}  // namespace system_manager
