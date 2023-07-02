

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

    renderer.update(ModelInfo{
        .model_name = usesCharacterModel.fetch_model(),
        .size_scale = 1.5f,
        .position_offset = vec3{0, 0, 0},
        .rotation_angle = 180,
    });

    usesCharacterModel.mark_change_completed();
}

inline bool render_simple_highlighted(std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing<Transform>()) return false;
    Transform& transform = entity->get<Transform>();
    if (entity->is_missing<SimpleColoredBoxRenderer>()) return false;
    SimpleColoredBoxRenderer& renderer =
        entity->get<SimpleColoredBoxRenderer>();
    // TODO replace size with Bounds component when it exists
    draw_valid_colored_box(transform, renderer, true);
    return true;
}

inline bool render_simple_normal(std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing<Transform>()) return false;
    Transform& transform = entity->get<Transform>();
    if (entity->is_missing<SimpleColoredBoxRenderer>()) return false;
    SimpleColoredBoxRenderer& renderer =
        entity->get<SimpleColoredBoxRenderer>();

    draw_valid_colored_box(transform, renderer, false);
    return true;
}

inline bool render_bounding_box(std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing<Transform>()) return false;
    Transform& transform = entity->get<Transform>();

    DrawBoundingBox(transform.bounds(), MAROON);
    DrawFloatingText(transform.raw(), Preload::get().font,
                     fmt::format("{}", entity->id).c_str());
    return true;
}

inline bool render_debug(std::shared_ptr<Entity> entity, float dt) {
    job_system::render_job_visual(entity, dt);

    // Ghost player only render during debug mode
    if (entity->has<CanBeGhostPlayer>() &&
        entity->get<CanBeGhostPlayer>().is_ghost()) {
        render_simple_normal(entity, dt);
    }
    return render_bounding_box(entity, dt);
}

inline bool render_model_highlighted(std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing<ModelRenderer>()) return false;
    if (entity->is_missing<CanBeHighlighted>()) return false;

    ModelRenderer& renderer = entity->get<ModelRenderer>();
    if (!renderer.has_model()) return false;

    if (entity->is_missing<Transform>()) return false;
    Transform& transform = entity->get<Transform>();

    ModelInfo model_info = renderer.model_info().value();

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

inline bool render_model_normal(std::shared_ptr<Entity> entity, float) {
    if (!ENABLE_MODELS) return false;

    if (entity->is_missing<ModelRenderer>()) return false;

    ModelRenderer& renderer = entity->get<ModelRenderer>();
    if (!renderer.has_model()) return false;

    if (entity->is_missing<Transform>()) return false;
    Transform& transform = entity->get<Transform>();

    ModelInfo model_info = renderer.model_info().value();

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

inline void render_trigger_area(std::shared_ptr<Entity> entity, float dt) {
    if (entity->is_missing<IsTriggerArea>()) return;

    // TODO add highlight when you walk in
    // TODO add progress bar when all players are inside

    const IsTriggerArea& ita = entity->get<IsTriggerArea>();

    const Transform& transform = entity->get<Transform>();

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

    std::string title = entity->get<IsTriggerArea>().title();
    log_ifx(title.empty(), LogLevel::LOG_WARN,
            "Rendering trigger area with empty text string: id{} pos{}",
            entity->id, pos);

    if (ita.should_wave()) {
        raylib::WaveTextConfig waveConfig = {.waveRange = {0, 0, TILESIZE},
                                             .waveSpeed = {0, 0, 0.01f},
                                             .waveOffset = {0.f, 0.f, 0.2f}};
        // TODO we probably should have all of this kind of thing in the config
        // for the components
        //
        raylib::DrawTextWave3D(font,
                               fmt::format("~~{}~~", "Loading...").c_str(),
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

inline void render_speech_bubble(std::shared_ptr<Entity> entity, float) {
    // Right now this is the only thing we can put in a bubble
    if (entity->is_missing<CanHaveAilment>()) return;
    if (entity->get<HasSpeechBubble>().disabled()) return;

    const Transform& transform = entity->get<Transform>();
    const vec3 position = transform.pos();

    const CanHaveAilment& cha = entity->get<CanHaveAilment>();
    auto ailment = cha.ailment();
    if (!ailment) return;

    GameCam cam = GLOBALS.get<GameCam>("game_cam");
    raylib::Texture texture = TextureLibrary::get().get(ailment->icon_name());
    raylib::DrawBillboard(cam.camera, texture,
                          vec3{position.x,                     //
                               position.y + (TILESIZE * 2.f),  //
                               position.z},                    //
                          TILESIZE,                            //
                          raylib::WHITE);
}

inline void render_normal(std::shared_ptr<Entity> entity, float dt) {
    // Ghost player cant render during normal mode
    if (entity->has<CanBeGhostPlayer>() &&
        entity->get<CanBeGhostPlayer>().is_ghost()) {
        return;
    }

    if (entity->has<IsTriggerArea>()) {
        render_trigger_area(entity, dt);
        return;
    }

    if (entity->has<CanBeHighlighted>() &&
        entity->get<CanBeHighlighted>().is_highlighted()) {
        bool used = render_model_highlighted(entity, dt);
        if (!used) {
            render_simple_highlighted(entity, dt);
        }
        return;
    }

    if (entity->has<HasSpeechBubble>()) {
        render_speech_bubble(entity, dt);
    }

    bool used = render_model_normal(entity, dt);
    if (!used) {
        render_simple_normal(entity, dt);
    }
}

inline void render_floating_name(std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing<HasName>()) return;
    HasName& hasName = entity->get<HasName>();

    if (entity->is_missing<Transform>()) return;
    const Transform& transform = entity->get<Transform>();

    // log_warn("drawing floating name {} for {} @ {} ({})", hasName.name(),
    // entity->id, transform.position, transform.raw_position);

    raylib::DrawFloatingText(transform.raw() + vec3{0, 0.5f * TILESIZE, 0},
                             Preload::get().font, hasName.name().c_str());
}

inline void render_progress_bar(std::shared_ptr<Entity> entity, float) {
    // TODO only renders for the host...

    if (entity->is_missing<ShowsProgressBar>()) return;

    if (entity->is_missing<Transform>()) return;
    const Transform& transform = entity->get<Transform>();

    if (entity->is_missing<HasWork>()) return;

    HasWork& hasWork = entity->get<HasWork>();
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

}  // namespace system_manager
