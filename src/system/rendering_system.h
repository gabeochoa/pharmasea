

#pragma once

#include "../engine/log.h"
#include "job_system.h"

namespace system_manager {
namespace render_manager {

inline bool render_simple_highlighted(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<Transform>()) return false;
    Transform& transform = entity->get<Transform>();
    if (!entity->has<SimpleColoredBoxRenderer>()) return false;
    SimpleColoredBoxRenderer& renderer =
        entity->get<SimpleColoredBoxRenderer>();

    Color f = ui::color::getHighlighted(renderer.face_color);
    Color b = ui::color::getHighlighted(renderer.base_color);
    // TODO replace size with Bounds component when it exists
    DrawCubeCustom(transform.raw_position, transform.size.x, transform.size.y,
                   transform.size.z,
                   transform.FrontFaceDirectionMap.at(transform.face_direction),
                   f, b);
    return true;
}

inline bool render_simple_normal(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<Transform>()) return false;
    Transform& transform = entity->get<Transform>();
    if (!entity->has<SimpleColoredBoxRenderer>()) return false;
    SimpleColoredBoxRenderer& renderer =
        entity->get<SimpleColoredBoxRenderer>();
    DrawCubeCustom(transform.raw_position, transform.size.x, transform.size.y,
                   transform.size.z,
                   transform.FrontFaceDirectionMap.at(transform.face_direction),
                   renderer.face_color, renderer.base_color);
    return true;
}

inline bool render_bounding_box(std::shared_ptr<Entity> entity, float dt) {
    if (!entity->has<Transform>()) return false;
    Transform& transform = entity->get<Transform>();

    DrawBoundingBox(transform.bounds(), MAROON);
    DrawFloatingText(transform.raw_position, Preload::get().font,
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
    if (!entity->has<ModelRenderer>()) return false;
    if (!entity->has<CanBeHighlighted>()) return false;

    ModelRenderer& renderer = entity->get<ModelRenderer>();
    if (!renderer.has_model()) return false;

    if (!entity->has<Transform>()) return false;
    Transform& transform = entity->get<Transform>();

    ModelInfo model_info = renderer.model_info().value();
    Color base = ui::color::getHighlighted(WHITE /*this->base_color*/);

    float rotation_angle =
        // TODO make this api better
        180.f + static_cast<int>(transform.FrontFaceDirectionMap.at(
                    transform.face_direction));

    DrawModelEx(renderer.model(),
                {
                    transform.position.x + model_info.position_offset.x,
                    transform.position.y + model_info.position_offset.y,
                    transform.position.z + model_info.position_offset.z,
                },
                vec3{0.f, 1.f, 0.f}, rotation_angle,
                transform.size * model_info.size_scale, base);

    return true;
}

inline bool render_model_normal(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<ModelRenderer>()) return false;

    ModelRenderer& renderer = entity->get<ModelRenderer>();
    if (!renderer.has_model()) return false;

    if (!entity->has<Transform>()) return false;
    Transform& transform = entity->get<Transform>();

    ModelInfo model_info = renderer.model_info().value();

    float rotation_angle =
        // TODO make this api better
        180.f + static_cast<int>(transform.FrontFaceDirectionMap.at(
                    transform.face_direction));

    raylib::DrawModelEx(
        renderer.model(),
        {
            transform.position.x + model_info.position_offset.x,
            transform.position.y + model_info.position_offset.y,
            transform.position.z + model_info.position_offset.z,
        },
        vec3{0, 1, 0}, model_info.rotation_angle + rotation_angle,
        transform.size * model_info.size_scale, WHITE /*this->base_color*/);

    return true;
}

inline void render_normal(std::shared_ptr<Entity> entity, float dt) {
    // Ghost player cant render during normal mode
    if (entity->has<CanBeGhostPlayer>() &&
        entity->get<CanBeGhostPlayer>().is_ghost()) {
        return;
    }

    if (entity->has<CanBeHighlighted>() &&
        entity->get<CanBeHighlighted>().is_highlighted) {
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

}  // namespace render_manager

}  // namespace system_manager
