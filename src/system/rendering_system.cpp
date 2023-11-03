
#include "rendering_system.h"

#include "../components/has_client_id.h"
#include "../components/has_patience.h"
#include "../components/has_waiting_queue.h"
#include "../components/is_free_in_store.h"
#include "../components/is_progression_manager.h"
#include "../drawing_util.h"
#include "../engine/texture_library.h"
#include "../engine/ui/theme.h"
#include "../preload.h"
#include "system_manager.h"

extern ui::UITheme UI_THEME;

namespace system_manager {
namespace render_manager {

struct ProgressBarConfig {
    enum Type { Horizontal, Vertical } type = Horizontal;
    vec3 position = {0, 0, 0};
    vec3 scale = {1, 1, 1};
    float pct_full = 1.f;
    float y_offset = -1;
    float x_offset = -1;
    float pct_warning = -1.f;
    float pct_error = -1.f;
};

void DrawProgressBar(const ProgressBarConfig& config) {
    // TODO :WARNING_COLORS: Eventually merge this with the one in
    // ui_rendering_system
    Color primary = ui::UI_THEME.from_usage(ui::theme::Usage::Primary);
    if (config.pct_warning != -1.f && config.pct_full < config.pct_warning) {
        primary = ui::UI_THEME.from_usage(ui::theme::Usage::Accent);
    }
    if (config.pct_error != -1.f && config.pct_full < config.pct_error) {
        primary = ui::UI_THEME.from_usage(ui::theme::Usage::Error);
    }
    Color background = ui::UI_THEME.from_usage(ui::theme::Usage::Background);

    float y_offset =
        config.y_offset != -1 ? config.y_offset : 1.75f * config.scale.x;
    float x_offset = config.x_offset != -1 ? config.x_offset : 0;
    const vec3 pos = config.position;

    if (config.type == ProgressBarConfig::Horizontal) {
        raylib::rlPushMatrix();
        {
            raylib::rlTranslatef(pos.x, pos.y, pos.z);

            vec3 size = {
                config.scale.x,
                config.scale.y / 3.f,
                config.scale.z / 10.f,
            };
            const float x_size =
                fmax(0.000001f, util::lerp(0, size.x, config.pct_full));

            DrawCubeCustom(
                {
                    x_offset + (x_size / 2.f) - (config.scale.x / 4.f),
                    y_offset,  //
                    0          //
                },
                x_size, size.y, size.z, 0, primary, primary);

            DrawCubeCustom(
                {
                    x_offset + (x_size / 2.f) + (config.scale.x / 4.f),
                    y_offset,  //
                    0          //
                },
                size.x - x_size, size.y, size.z, 0, background, background);
        }
        raylib::rlPopMatrix();
        return;
    }

    raylib::rlPushMatrix();
    {
        raylib::rlTranslatef(pos.x, pos.y, pos.z);

        vec3 size = {
            config.scale.x / 3.f,
            config.scale.y,
            config.scale.z / 10.f,
        };

        const float y_size =
            fmax(0.000001f, util::lerp(0, size.y, config.pct_full));

        DrawCubeCustom(
            {
                x_offset,  //
                y_offset + (y_size / 2.f) - (config.scale.y / 4.f),
                0  //
            },
            size.x, y_size, size.z, 0, primary, primary);

        DrawCubeCustom(
            {
                x_offset,  //
                y_offset + (y_size / 2.f) + (config.scale.y / 4.f),
                0  //
            },
            size.x, size.y - y_size, size.z, 0, background, background);
    }
    raylib::rlPopMatrix();
}

void draw_valid_colored_box(const Transform& transform,
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

    DrawCubeCustom(transform.raw(), transform.sizex(), transform.sizey(),
                   transform.sizez(), transform.facing, f, b);
}

void update_character_model_from_index(Entity& entity, float) {
    if (entity.is_missing<UsesCharacterModel>()) return;
    UsesCharacterModel& usesCharacterModel = entity.get<UsesCharacterModel>();

    if (usesCharacterModel.value_same_as_last_render()) return;

    if (entity.is_missing<ModelRenderer>()) return;

    ModelRenderer& renderer = entity.get<ModelRenderer>();

    // TODO this should be the same as all other rendere updates for players
    renderer.update_model_name(usesCharacterModel.fetch_model_name());
    usesCharacterModel.mark_change_completed();
}

bool render_simple_highlighted(const Entity& entity, float) {
    if (entity.is_missing<Transform>()) return false;
    const Transform& transform = entity.get<Transform>();
    if (entity.is_missing<SimpleColoredBoxRenderer>()) return false;
    const SimpleColoredBoxRenderer& renderer =
        entity.get<SimpleColoredBoxRenderer>();
    // TODO replace size with Bounds component when it exists
    draw_valid_colored_box(transform, renderer, true);
    return true;
}

bool render_simple_normal(const Entity& entity, float) {
    if (entity.is_missing<Transform>()) return false;
    const Transform& transform = entity.get<Transform>();
    if (entity.is_missing<SimpleColoredBoxRenderer>()) return false;
    const SimpleColoredBoxRenderer& renderer =
        entity.get<SimpleColoredBoxRenderer>();

    draw_valid_colored_box(transform, renderer, false);
    return true;
}

bool render_bounding_box(const Entity& entity, float) {
    if (entity.is_missing<Transform>()) return false;
    const Transform& transform = entity.get<Transform>();

    DrawFloatingText(transform.raw(), Preload::get().font,
                     fmt::format("{}", entity.id).c_str());

    if (false && check_type(entity, EntityType::RemotePlayer)) {
        vec3 circle_bounds = transform.circular_bounds();
        DrawSphere({circle_bounds.x, -1.f * (TILESIZE / 2.f), circle_bounds.y},
                   circle_bounds.z, MAROON);
    } else {
        DrawBoundingBox(transform.bounds(), MAROON);
        Rectangle rect_bounds = transform.rectangular_bounds();
        DrawCubeCustom({rect_bounds.x, -1.f * (TILESIZE / 2.f), rect_bounds.y},
                       rect_bounds.width, TILESIZE / 7.f, rect_bounds.height,
                       transform.facing, MAROON, MAROON);
    }

    return true;
}

void render_debug_subtype(const Entity& entity, float) {
    const Transform& transform = entity.get<Transform>();
    std::string content;

    if (entity.has<HasSubtype>()) {
        const HasSubtype& hs = entity.get<HasSubtype>();
        content = fmt::format("{}", hs.get_type_index());
        // Convert from ID to ingredient if its an alcohol
        if (check_type(entity, EntityType::Alcohol)) {
            content = fmt::format("{}", hs.as_type<Ingredient>());
        }
    }

    if (entity.has<IsPnumaticPipe>()) {
        const IsPnumaticPipe& ipp = entity.get<IsPnumaticPipe>();
        content = fmt::format("{} -> {}", entity.id, ipp.paired_id);
    }

    DrawFloatingText(vec::raise(transform.raw(), 1.f), Preload::get().font,
                     content.c_str());
}

void render_debug_drink_info(const Entity& entity, float) {
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

void render_debug_filter_info(const Entity& entity, float) {
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

bool render_debug(const Entity& entity, float dt) {
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

bool render_model_highlighted(const Entity& entity, float) {
    if (entity.is_missing<ModelRenderer>()) return false;
    if (entity.is_missing<CanBeHighlighted>()) return false;
    if (entity.is_missing<Transform>()) return false;

    const Transform& transform = entity.get<Transform>();
    const ModelRenderer& renderer = entity.get<ModelRenderer>();
    if (renderer.missing()) return false;
    ModelInfo& model_info = renderer.model_info();

    // TODO this is the exact same code as render_model_normal
    // should be able to fix it

    float rotation_angle = 180.f + transform.facing;

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

bool render_model_normal(const Entity& entity, float) {
    if (!ENABLE_MODELS) return false;
    if (entity.is_missing<ModelRenderer>()) return false;
    if (entity.is_missing<Transform>()) return false;
    const Transform& transform = entity.get<Transform>();

    const ModelRenderer& renderer = entity.get<ModelRenderer>();
    if (renderer.missing()) return false;

    const ModelInfo& model_info = renderer.model_info();

    float rotation_angle = 180.f + transform.facing;

    raylib::DrawModelEx(
        renderer.model(),
        {
            transform.pos().x + model_info.position_offset.x,
            transform.pos().y + model_info.position_offset.y,
            transform.pos().z + model_info.position_offset.z,
        },
        vec3{0, 1, 0}, model_info.rotation_angle + rotation_angle,
        transform.size() * model_info.size_scale, WHITE /*base_color*/);

    return true;
}

void render_trigger_area(const Entity& entity, float dt) {
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
        raylib::WaveTextConfig waveConfig = {.waveRange = {0, 0, 1},
                                             .waveSpeed = {0.0f, 0.0f, 0.01f},
                                             .waveOffset = {0.f, 0.f, 0.2f}};
        // TODO we probably should have all of this kind of thing in the config
        // for the components
        //
        raylib::DrawTextWave3D(
            font,
            fmt::format("~~{}~~", text_lookup(ita.subtitle().c_str())).c_str(),
            text_position, fsize,
            4,                      // font spacing
            4,                      // line spacing
            false,                  // backface
            &waveConfig,            //
            now::current_hrc_ms(),  //
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
            4,      // font spacing
            4,      // line spacing
            false,  // backface
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
            size.x * ita.progress(),  //
            size.y,                   //
            size.z, transform.facing, RED, RED);
    }

    render_simple_normal(entity, dt);

    const auto _render_drink_preview = [&](IsTriggerArea::Type type) {
        if (GameState::get().is_not(game::State::Progression)) return;

        OptEntity sophie = EntityHelper::getFirstOfType(EntityType::Sophie);
        const IsProgressionManager& ipm = sophie->get<IsProgressionManager>();
        Drink drink = type == IsTriggerArea::Progression_Option1 ? ipm.option1
                                                                 : ipm.option2;

        const auto font = Preload::get().font;
        const auto start_position =
            transform.raw() + vec3{0, 1.0f * TILESIZE, -2.f * TILESIZE};

        raylib::DrawFloatingText(start_position, font,
                                 get_string_for_drink(drink).c_str());

        auto ingredients = get_recipe_for_drink(drink);
        int i = 0;

        bitset_utils::for_each_enabled_bit(ingredients, [&](size_t bit) {
            Ingredient ig = magic_enum::enum_value<Ingredient>(bit);
            raylib::DrawFloatingText(
                start_position - vec3{0, 0.3f * (i + 1), 0}, font,
                fmt::format("{}", get_string_for_ingredient(ig)).c_str());
            i++;
        });
    };

    switch (ita.type) {
        case IsTriggerArea::Unset:
        case IsTriggerArea::Lobby_PlayGame:
        case IsTriggerArea::Store_BackToPlanning:
            break;
        case IsTriggerArea::Progression_Option1:
        case IsTriggerArea::Progression_Option2:
            _render_drink_preview(ita.type);
            break;
    }
}

void render_floor_marker(const Entity& entity, float) {
    if (entity.is_missing<IsFloorMarker>()) return;

    const Transform& transform = entity.get<Transform>();

    vec3 pos = transform.pos();
    vec3 size = transform.size();

    vec3 text_position = {pos.x - (size.x / 2.f),     //
                          pos.y + (TILESIZE / 10.f),  //
                          pos.z - (size.z / 3.f)};

    auto font = Preload::get().font;
    // TODO eventually we can detect the size to fit the text correctly
    // but thats more of an issue for translations since i can manually
    // place the english
    auto fsize = 500.f;

    const auto _get_string = [](const Entity& entity) {
        switch (entity.get<IsFloorMarker>().type) {
            case IsFloorMarker::Unset:
                return "";
            // TODO for not use the other one but eventually probably just make
            // it invisible
            case IsFloorMarker::Store_SpawnArea:
            case IsFloorMarker::Planning_SpawnArea:
                return text_lookup(strings::i18n::FLOORMARKER_NEW_ITEMS);
            case IsFloorMarker::Planning_TrashArea:
                return text_lookup(strings::i18n::FLOORMARKER_TRASH);
            case IsFloorMarker::Store_PurchaseArea:
                return text_lookup(strings::i18n::FLOORMARKER_STORE_PURCHASE);
        }
        return "";
    };

    const std::string title = _get_string(entity);

    log_ifx(title.empty(), LogLevel::LOG_WARN,
            "Rendering trigger area with empty text string: id{} pos{}",
            entity.id, pos);

    raylib::DrawText3D(font, title.c_str(), text_position, fsize,
                       4,      // font spacing
                       4,      // line spacing
                       false,  // backface
                       WHITE);

    // we dont need this since the caller of render_floor_marker doesnt
    // return render_simple_normal(entity, dt);

    // Next lets draw the trash marker
    const IsFloorMarker& ifm = entity.get<IsFloorMarker>();
    if (ifm.type == IsFloorMarker::Type::Planning_TrashArea) {
        for (size_t i = 0; i < ifm.num_marked(); i++) {
            EntityID id = ifm.marked_ids()[i];
            OptEntity marked_entity = EntityHelper::getEntityForID(id);
            if (!marked_entity) continue;
            render_trash_marker(marked_entity.asE());
        }
    }
    if (ifm.type == IsFloorMarker::Type::Store_PurchaseArea) {
        for (size_t i = 0; i < ifm.num_marked(); i++) {
            EntityID id = ifm.marked_ids()[i];
            OptEntity marked_entity = EntityHelper::getEntityForID(id);
            if (!marked_entity) continue;
            render_dollarsign_marker(marked_entity.asE());
        }
    }
}

void render_dollarsign_marker(const Entity& entity) {
    const Transform& transform = entity.get<Transform>();
    vec3 position = transform.pos();

    raylib::Texture texture = TextureLibrary::get().get("dollar_sign");
    GameCam cam = GLOBALS.get<GameCam>(strings::globals::GAME_CAM);
    raylib::DrawBillboard(cam.camera, texture,
                          vec3{position.x + (TILESIZE * 0.05f),  //
                               position.y + (TILESIZE * 2.f),    //
                               position.z},                      //
                          0.75f * TILESIZE,                      //
                          raylib::WHITE);
}

void render_trash_marker(const Entity& entity) {
    const Transform& transform = entity.get<Transform>();
    vec3 position = transform.pos();

    raylib::Texture texture = TextureLibrary::get().get("trashcan");
    GameCam cam = GLOBALS.get<GameCam>(strings::globals::GAME_CAM);
    raylib::DrawBillboard(cam.camera, texture,
                          vec3{position.x + (TILESIZE * 0.05f),  //
                               position.y + (TILESIZE * 2.f),    //
                               position.z},                      //
                          0.75f * TILESIZE,                      //
                          raylib::WHITE);
}

void render_patience(const Entity& entity, float) {
    if (entity.is_missing<HasPatience>()) return;
    const Transform& transform = entity.get<Transform>();
    const HasPatience& hp = entity.get<HasPatience>();
    if (hp.pct() >= 1.f) return;

    DrawProgressBar(
        ProgressBarConfig{.type = ProgressBarConfig::Vertical,
                          .position = transform.pos(),
                          .scale = {0.75f, 0.75f, 0.75f},
                          .pct_full = hp.pct(),
                          .y_offset = 2.f * TILESIZE,
                          .x_offset = -0.45f * TILESIZE,
                          // dont change this numbers without updating
                          // the ones in ui_rendering_system
                          .pct_warning = 0.2f,
                          .pct_error = 0.05f});
}

void render_speech_bubble(const Entity& entity, float) {
    if (entity.is_missing<HasSpeechBubble>()) return;
    if (entity.get<HasSpeechBubble>().disabled()) return;

    // Right now this is the only thing we can put in a bubble
    if (entity.is_missing<CanOrderDrink>()) return;

    const Transform& transform = entity.get<Transform>();
    const vec3 position = transform.pos();

    const CanOrderDrink& cod = entity.get<CanOrderDrink>();

    const vec3 icon_position = vec3{position.x + (TILESIZE * 0.05f),  //
                                    position.y + (TILESIZE * 2.f),    //
                                    position.z};

    // TODO add way to turn on / off these icons
    if (true) {
        GameCam cam = GLOBALS.get<GameCam>(strings::globals::GAME_CAM);
        raylib::Texture texture = TextureLibrary::get().get(cod.icon_name());
        raylib::DrawBillboard(cam.camera, texture,
                              // move it a bit so that it doesnt overlap
                              icon_position + vec3{0.f, 1.f, 0},
                              0.75f * TILESIZE, raylib::WHITE);
    }

    const auto model_name = get_model_name_for_drink(cod.current_order);
    const auto model = ModelLibrary::get().get(model_name);
    const ModelInfo& model_info = ModelInfoLibrary::get().get(model_name);

    float rotation_angle = 180.f + transform.facing;
    raylib::DrawModelEx(
        model, icon_position + model_info.position_offset, vec3{0, 1, 0},
        model_info.rotation_angle + rotation_angle,
        transform.size() * model_info.size_scale, WHITE /*base_color*/);
}

void render_waiting_queue(const Entity& entity, float) {
    if (entity.is_missing<HasWaitingQueue>()) return;

    // TODO this keep crashing the game if you put the pumpkin holder
    // somewhere else? im wondering if it has to do with the server thread
    // killing the cache?
    return;

    const HasWaitingQueue& hwq = entity.get<HasWaitingQueue>();

    const Transform& transform = entity.get<Transform>();
    vec3 size = transform.size();

    // TODO spelling?
    Color transleucent_green = Color{0, 250, 50, 50};
    Color transleucent_red = Color{250, 0, 50, 50};

    for (size_t i = 0; i < hwq.max_queue_size; i++) {
        vec2 pos2 = transform.tile_infront((int) i + 1);
        vec3 pos = vec::to3(pos2);
        bool walkable = EntityHelper::isWalkable(pos2);
        DrawCubeCustom({pos.x, pos.y - (TILESIZE * 0.5f), pos.z}, size.x,
                       size.y / 10.f, size.z, transform.facing,
                       walkable ? transleucent_green : transleucent_red,
                       walkable ? transleucent_green : transleucent_red);
    }
}

void render_price(const Entity& entity, float) {
    int price = get_price_for_entity_type(entity.get<DebugName>().get_type());
    if (price == -1) return;

    if (entity.is_missing<Transform>()) return;
    const Transform& transform = entity.get<Transform>();

    vec3 location = transform.raw() + vec3{0, 0.5f * TILESIZE, 0.4f * TILESIZE};

    bool is_free = entity.has<IsFreeInStore>();
    if (is_free) price = 0;

    // TODO rotate the name with the camera?
    raylib::DrawFloatingText(location, Preload::get().font,
                             fmt::format("${}", price).c_str(), 200,
                             is_free ? GREEN : BLUE);
}

void render_machine_name(const Entity& entity, float) {
    if (entity.is_missing<IsSolid>()) return;
    if (entity.is_missing<Transform>()) return;
    const Transform& transform = entity.get<Transform>();

    const auto machine_name =
        magic_enum::enum_name<EntityType>(entity.get<DebugName>().get_type());

    // TODO rotate the name with the camera?
    raylib::DrawFloatingText(
        transform.raw() + vec3{0, 1.0f * TILESIZE, 0.2f * TILESIZE},
        Preload::get().font, std::string(machine_name).c_str(), 200);
}

// TODO theres two functions called render normal, maybe we should address
// this
void render_normal(const Entity& entity, float dt) {
    //  TODO for now while we do dev work render it
    render_debug_subtype(entity, dt);
    render_debug_drink_info(entity, dt);
    render_debug_filter_info(entity, dt);

    render_waiting_queue(entity, dt);

    // Ghost player cant render during normal mode
    if (entity.has<CanBeGhostPlayer>() &&
        entity.get<CanBeGhostPlayer>().is_ghost()) {
        return;
    }

    if (check_type(entity, EntityType::Face)) {
        auto cam = GLOBALS.get_ptr<GameCam>(strings::globals::GAME_CAM);
        raylib::DrawBillboard(
            cam->camera, TextureLibrary::get().get(strings::textures::FACE),
            entity.get<Transform>().pos(), TILESIZE, WHITE);

        return;
    }

    if (entity.has<IsTriggerArea>()) {
        render_trigger_area(entity, dt);
        return;
    }

    if (entity.has<IsFloorMarker>()) {
        render_floor_marker(entity, dt);
    }

    if (GameState::get().is(game::State::Store)) {
        render_machine_name(entity, dt);
        render_price(entity, dt);
    }

    if (entity.has<CanBeHighlighted>() &&
        entity.get<CanBeHighlighted>().is_highlighted()) {
        bool used = render_model_highlighted(entity, dt);
        if (!used) {
            render_simple_highlighted(entity, dt);
        }
        return;
    }

    render_speech_bubble(entity, dt);
    render_patience(entity, dt);

    bool used = render_model_normal(entity, dt);
    if (!used) {
        render_simple_normal(entity, dt);
    }
}

void render_floating_name(const Entity& entity, float) {
    if (entity.is_missing<HasName>()) return;
    const HasName& hasName = entity.get<HasName>();

    if (entity.is_missing<Transform>()) return;
    const Transform& transform = entity.get<Transform>();

    // log_warn("drawing floating name {} for {} @ {}", hasName.name(),
    // entity.id, transform.pos());

    // TODO rotate the name with the camera?
    raylib::DrawFloatingText(
        transform.raw() + vec3{0, 0.5f * TILESIZE, 0.2f * TILESIZE},
        Preload::get().font, hasName.name().c_str());
}

void render_progress_bar(const Entity& entity, float) {
    if (entity.is_missing<ShowsProgressBar>()) return;

    const auto spr_type = entity.get<ShowsProgressBar>().type;
    game::State state = GameState::get().read();

    const auto _should_render = [&]() {
        if (spr_type == ShowsProgressBar::Enabled::Always) return true;
        if (state == game::State::InRound &&
            spr_type == ShowsProgressBar::Enabled::InRound)
            return true;
        if (state == game::State::Planning &&
            spr_type == ShowsProgressBar::Enabled::Planning)
            return true;
        return false;
    };

    if (!_should_render()) return;

    if (entity.is_missing<Transform>()) return;
    const Transform& transform = entity.get<Transform>();

    if (entity.is_missing<HasWork>()) return;

    const HasWork& hasWork = entity.get<HasWork>();
    if (hasWork.dont_show_progres_bar()) return;

    DrawProgressBar(ProgressBarConfig{
        .position = transform.pos(),
        .pct_full = hasWork.scale_length(1.f),
    });
}

void render_walkable_spots(float) {
    // TODO For some reason this also triggers the walkable.contains
    // segfault
    return;

    if (!GLOBALS.get<bool>("debug_ui_enabled")) return;

    // TODO spelling?
    Color transleucent_green = Color{0, 250, 50, 5};
    Color transleucent_red = Color{250, 0, 50, 5};

    for (int i = -25; i < 25; i++) {
        for (int j = -25; j < 25; j++) {
            vec2 pos2 = {(float) i, (float) j};
            bool walkable = EntityHelper::isWalkable(pos2);
            if (walkable) continue;

            DrawCubeCustom(vec::to3(pos2), TILESIZE, TILESIZE + TILESIZE / 10.f,
                           TILESIZE, 0,
                           walkable ? transleucent_green : transleucent_red,
                           walkable ? transleucent_green : transleucent_red);
        }
    }
}

void render_held_furniture_preview(const Entity& entity, float) {
    if (entity.is_missing<CanHoldFurniture>()) return;
    const CanHoldFurniture& chf = entity.get<CanHoldFurniture>();
    if (chf.empty()) return;
    const Transform& transform = entity.get<Transform>();

    vec3 drop_location = entity.get<Transform>().drop_location();
    bool walkable = EntityHelper::isWalkable(vec::to2(drop_location));

    walkable = walkable || (drop_location == chf.picked_up_at());

    // since the preview is just a box we can just use 0 for angle
    // but if we need in the future, then we can fetch the entity with:
    //
    // OptEntity hf = EntityHelper::getEntityForID(chf.furniture_id());
    // const Transform& furniture_transform = hf->get<Transform>();

    vec3 size = (transform.size() * 1.2f);
    DrawCubeCustom(drop_location, size.x, size.y, size.z, 0,
                   walkable ? GREEN : RED, walkable ? GREEN : RED);
}

}  // namespace render_manager
}  // namespace system_manager

namespace system_manager {
namespace ui {

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
                entity.get<DebugName>().name());
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

}  // namespace ui

namespace render_manager {

void render(const Entity& entity, float dt, bool is_debug) {
    if (is_debug) render_debug(entity, dt);

    render_normal(entity, dt);
    render_held_furniture_preview(entity, dt);
    render_floating_name(entity, dt);
    render_progress_bar(entity, dt);
}

}  // namespace render_manager

}  // namespace system_manager
