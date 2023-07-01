

#pragma once

#include "../components/uses_character_model.h"
#include "../engine/log.h"
#include "../engine/time.h"
#include "job_system.h"

namespace system_manager {
namespace render_manager {

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

    Color f = ui::color::getHighlighted(renderer.face());
    Color b = ui::color::getHighlighted(renderer.base());
    // TODO replace size with Bounds component when it exists
    DrawCubeCustom(
        transform.raw(), transform.sizex(), transform.sizey(),
        transform.sizez(),
        transform.FrontFaceDirectionMap.at(transform.face_direction()), f, b);
    return true;
}

inline bool render_simple_normal(std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing<Transform>()) return false;
    Transform& transform = entity->get<Transform>();
    if (entity->is_missing<SimpleColoredBoxRenderer>()) return false;
    SimpleColoredBoxRenderer& renderer =
        entity->get<SimpleColoredBoxRenderer>();

    // This logif exists because i had a problem where all furniture was
    // size(0,0,0) and took me a while to figure it out.
    log_ifx((transform.sizex() == 0 || transform.sizey() == 0 ||
             transform.sizez() == 0),
            LOG_WARN, "Trying to render entity {} that has size {}", entity->id,
            transform.size());
    //

    DrawCubeCustom(
        transform.raw(), transform.sizex(), transform.sizey(),
        transform.sizez(),
        transform.FrontFaceDirectionMap.at(transform.face_direction()),
        renderer.face(), renderer.base());
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
    Color base = ui::color::getHighlighted(WHITE /*this->base_color*/);

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
                vec3{0.f, 1.f, 0.f}, rotation_angle,
                transform.size() * model_info.size_scale, base);

    return true;
}

inline bool render_model_normal(std::shared_ptr<Entity> entity, float) {
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
                          pos.z - (size.z / 5.f)};
    // Top left text position
    // {pos.x - (size.x / 2.f),     //
    // pos.y + (TILESIZE / 20.f),  //
    // pos.z - (size.z / 2.f)};

    auto font = Preload::get().font;
    // TODO eventually we can detect the size to fit the text correctly
    // but thats more of an issue for translations since i can manually
    // place the english
    auto fsize = 300.f;

    if (ita.should_wave()) {
        raylib::WaveTextConfig waveConfig = {.waveRange = {0, 0, TILESIZE},
                                             .waveSpeed = {0, 0, 0.01f},
                                             .waveOffset = {0.f, 0.f, 0.2f}};
        raylib::DrawTextWave3D(
            font,
            fmt::format("~~{}~~", entity->get<IsTriggerArea>().title()).c_str(),
            text_position, fsize,
            4,                  // font spacing
            4,                  // line spacing
            false,              // backface
            &waveConfig,        //
            now::current_ms(),  //
            WHITE);

    } else {
        raylib::DrawText3D(font, entity->get<IsTriggerArea>().title().c_str(),
                           text_position, fsize,
                           4,      // font spacing
                           4,      // line spacing
                           false,  // backface
                           WHITE);
    }

    DrawCubeCustom(
        {pos.x, pos.y + (TILESIZE / 20.f), pos.z}, size.x, size.y,
        size.z * ita.progress(),
        transform.FrontFaceDirectionMap.at(transform.face_direction()), RED,
        RED);
    render_simple_normal(entity, dt);
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
