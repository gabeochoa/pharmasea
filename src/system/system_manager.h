
#pragma once

#include "../components/can_be_ghost_player.h"
#include "../components/can_highlight_others.h"
#include "../components/can_hold_furniture.h"
#include "../components/custom_item_position.h"
#include "../components/transform.h"
#include "../entity.h"
#include "../entityhelper.h"
#include "../furniture.h"

namespace system_manager {

inline void transform_snapper(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<Transform>()) return;

    Transform& transform = entity->get<Transform>();
    if (entity->is_snappable()) {
        transform.position = transform.snap_position();
    } else {
        transform.position = transform.raw_position;
    }
}

inline void update_held_furniture_position(std::shared_ptr<Entity> entity,
                                           float) {
    if (!entity->has<CanHoldFurniture>()) return;
    CanHoldFurniture& can_hold_furniture = entity->get<CanHoldFurniture>();

    // TODO explicity commenting this out so that we get an error
    // if (!entity->has<Transform>()) return;
    Transform& transform = entity->get<Transform>();

    // TODO if cannot be placed in this spot make it obvious to the user

    if (can_hold_furniture.empty()) return;

    auto new_pos = transform.position;
    if (transform.face_direction & Transform::FrontFaceDirection::FORWARD) {
        new_pos.z += TILESIZE;
    }
    if (transform.face_direction & Transform::FrontFaceDirection::RIGHT) {
        new_pos.x += TILESIZE;
    }
    if (transform.face_direction & Transform::FrontFaceDirection::BACK) {
        new_pos.z -= TILESIZE;
    }
    if (transform.face_direction & Transform::FrontFaceDirection::LEFT) {
        new_pos.x -= TILESIZE;
    }

    can_hold_furniture.furniture()->update_position(new_pos);
}

inline void update_held_item_position(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<CanHoldItem>()) return;
    CanHoldItem& can_hold_item = entity->get<CanHoldItem>();
    if (can_hold_item.empty()) return;

    if (!entity->has<Transform>()) return;
    Transform& transform = entity->get<Transform>();

    if (entity->has<CustomHeldItemPosition>()) {
        CustomHeldItemPosition& custom_item_position =
            entity->get<CustomHeldItemPosition>();

        if (custom_item_position.mutator) {
            can_hold_item.item()->update_position(
                custom_item_position.mutator(transform));
        } else {
            log_warn(
                "Entity has custom held item position but didnt initalize the "
                "component");
        }
        return;
    }

    // Default

    auto new_pos = transform.position;
    if (transform.face_direction & Transform::FrontFaceDirection::FORWARD) {
        new_pos.z += TILESIZE;
    }
    if (transform.face_direction & Transform::FrontFaceDirection::RIGHT) {
        new_pos.x += TILESIZE;
    }
    if (transform.face_direction & Transform::FrontFaceDirection::BACK) {
        new_pos.z -= TILESIZE;
    }
    if (transform.face_direction & Transform::FrontFaceDirection::LEFT) {
        new_pos.x -= TILESIZE;
    }
    can_hold_item.item()->update_position(new_pos);
}

inline void render_simple_highlighted(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<Transform>()) return;
    Transform& transform = entity->get<Transform>();
    if (!entity->has<SimpleColoredBoxRenderer>()) return;
    SimpleColoredBoxRenderer& renderer =
        entity->get<SimpleColoredBoxRenderer>();

    Color f = ui::color::getHighlighted(renderer.face_color);
    Color b = ui::color::getHighlighted(renderer.base_color);
    // TODO replace size with Bounds component when it exists
    DrawCubeCustom(transform.raw_position, transform.size.x, transform.size.y,
                   transform.size.z,
                   transform.FrontFaceDirectionMap.at(transform.face_direction),
                   f, b);
}

inline void render_simple_normal(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<Transform>()) return;
    Transform& transform = entity->get<Transform>();
    if (!entity->has<SimpleColoredBoxRenderer>()) return;
    SimpleColoredBoxRenderer& renderer =
        entity->get<SimpleColoredBoxRenderer>();
    DrawCubeCustom(transform.raw_position, transform.size.x, transform.size.y,
                   transform.size.z,
                   transform.FrontFaceDirectionMap.at(transform.face_direction),
                   renderer.face_color, renderer.base_color);
}

inline void render_debug(std::shared_ptr<Entity> entity, float dt) {
    // Ghost player only render during debug mode
    if (entity->has<CanBeGhostPlayer>() &&
        entity->get<CanBeGhostPlayer>().is_not_ghost()) {
        return;
    }
    if (entity->has<CanBeHighlighted>() &&
        entity->get<CanBeHighlighted>().is_highlighted) {
        render_simple_highlighted(entity, dt);
        return;
    }

    render_simple_normal(entity, dt);
}

inline void render_normal(std::shared_ptr<Entity> entity, float dt) {
    // Ghost player cant render during normal mode
    if (entity->has<CanBeGhostPlayer>() &&
        entity->get<CanBeGhostPlayer>().is_ghost()) {
        return;
    }

    if (entity->has<CanBeHighlighted>() &&
        entity->get<CanBeHighlighted>().is_highlighted) {
        render_simple_highlighted(entity, dt);
        return;
    }

    render_simple_normal(entity, dt);
}

inline void reset_highlighted(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<CanBeHighlighted>()) return;
    CanBeHighlighted& cbh = entity->get<CanBeHighlighted>();
    cbh.is_highlighted = false;
}

inline void highlight_facing_furniture(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<CanHighlightOthers>()) return;
    CanHighlightOthers& cho = entity->get<CanHighlightOthers>();

    // TODO explicity commenting this out so that we get an error
    // if (!entity->has<Transform>()) return;
    Transform& transform = entity->get<Transform>();

    // TODO this is impossible to read, what can we do to fix this while
    // keeping it configurable
    auto match = EntityHelper::getMatchingEntityInFront<Furniture>(
        // TODO add a player reach component
        transform.as2(), cho.reach(), transform.face_direction,
        [](std::shared_ptr<Furniture>) { return true; });
    if (!match) return;
    if (!match->has<CanBeHighlighted>()) return;
    match->get<CanBeHighlighted>().is_highlighted = true;
}

}  // namespace system_manager

struct SystemManager {
    void always_update(float dt) {
        EntityHelper::forEachEntity([dt](std::shared_ptr<Entity> entity) {
            system_manager::reset_highlighted(entity, dt);
            system_manager::transform_snapper(entity, dt);
            system_manager::update_held_item_position(entity, dt);
            return EntityHelper::ForEachFlow::None;
        });
    }

    void in_round_update(float dt) {
        EntityHelper::forEachEntity([dt](std::shared_ptr<Entity> entity) {
            return EntityHelper::ForEachFlow::None;
        });
    }

    void planning_update(float dt) {
        EntityHelper::forEachEntity([dt](std::shared_ptr<Entity> entity) {
            system_manager::highlight_facing_furniture(entity, dt);
            system_manager::update_held_furniture_position(entity, dt);
            return EntityHelper::ForEachFlow::None;
        });
    }

    void render_normal(float dt) const {
        EntityHelper::forEachEntity([dt](std::shared_ptr<Entity> entity) {
            system_manager::render_normal(entity, dt);
            return EntityHelper::ForEachFlow::None;
        });
    }

    void render_debug(float dt) const {
        EntityHelper::forEachEntity([dt](std::shared_ptr<Entity> entity) {
            system_manager::render_debug(entity, dt);
            return EntityHelper::ForEachFlow::None;
        });
    }
};
