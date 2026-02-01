
#include "entity_query.h"

#include "components/can_hold_furniture.h"
#include "components/can_hold_item.h"
#include "components/is_drink.h"
#include "engine/assert.h"
#include "engine/pathfinder.h"
#include "entity_helper.h"

EQ::EQ(const EQ& other)
    : afterhours::EntityQuery<EQ>(EntityHelper::get_current_collection(),
                                  {.ignore_temp_warning = true}) {
    // Copy filter by re-running on same entity set
    // Note: We can't deep-copy mods, so we capture the entity IDs
    // from the original query and filter by those IDs
    auto ids = const_cast<EQ&>(other).gen_ids();
    std::set<int> id_set(ids.begin(), ids.end());
    add_mod(new WhereLambda([id_set = std::move(id_set)](const Entity& entity) {
        return id_set.count(entity.id) > 0;
    }));
}

bool EQ::WhereCanPathfindTo::operator()(const Entity& entity) const {
    return !pathfinder::find_path(
                start, entity.get<Transform>().tile_directly_infront(),
                std::bind(EntityHelper::isWalkable, std::placeholders::_1))
                .empty();
}

EQ& EQ::whereIsHoldingAnyFurniture() {
    return  //
        add_mod(new WhereHasComponent<CanHoldFurniture>())
            .add_mod(new WhereLambda([](const Entity& entity) {
                const CanHoldFurniture& chf = entity.get<CanHoldFurniture>();
                return chf.is_holding();
            }));
}

EQ& EQ::whereIsHoldingAnyFurnitureThatMatches(
    const std::function<bool(const Entity&)>& filter) {
    return  //
        add_mod(new WhereHasComponent<CanHoldFurniture>())
            .add_mod(new WhereLambda([&](const Entity& entity) {
                const CanHoldFurniture& chf = entity.get<CanHoldFurniture>();
                if (!chf.is_holding()) return false;
                OptEntity furniture =
                    EntityHelper::getEntityForID(chf.held_id());
                if (!furniture.has_value()) return false;

                return filter(furniture.asE());
            }));
}

EQ& EQ::whereIsHoldingFurnitureID(EntityID entityID) {
    return  //
        add_mod(new WhereHasComponent<CanHoldFurniture>())
            .add_mod(new WhereLambda([entityID](const Entity& entity) {
                const CanHoldFurniture& chf = entity.get<CanHoldFurniture>();
                return chf.is_holding() &&
                       chf.held_id() == entityID;
            }));
}

EQ& EQ::whereIsHoldingItemOfType(EntityType type) {
    return  //
        add_mod(new WhereHasComponent<CanHoldItem>())
            .add_mod(new WhereLambda([type](const Entity& entity) {
                const CanHoldItem& chi = entity.get<CanHoldItem>();
                if (!chi.is_holding_item()) return false;
                OptEntity held_opt = chi.item();
                return held_opt && held_opt->hasTag(type);
            }));
}

EQ& EQ::whereIsDrinkAndMatches(Drink recipe) {
    return  //
        add_mod(new WhereHasComponent<IsDrink>())
            .add_mod(new WhereLambda([recipe](const Entity& entity) {
                return entity.get<IsDrink>().matches_drink(recipe);
            }));
}

EQ& EQ::whereHeldItemMatches(const std::function<bool(const Entity&)>& fn) {
    return  //
        add_mod(new WhereLambda([&fn](const Entity& entity) -> bool {
            const CanHoldItem& chf = entity.get<CanHoldItem>();
            if (!chf.is_holding_item()) return false;
            OptEntity held_opt = chf.const_item();
            if (!held_opt) return false;
            return fn(held_opt.asE());
        }));
}

OptEntity EQ::gen_closestInFront(const Transform& transform, float range) {
    TRACY_ZONE_SCOPED;
    VALIDATE(range > 0, fmt::format("range has to be positive but was {}", range));

    vec2 pos = transform.as2();
    auto direction = transform.face_direction();

    // First, get all entities matching the accumulated filters
    auto matching = gen();

    // Now search through positions from closest to farthest
    int irange = static_cast<int>(range);
    for (int cur_step = 0; cur_step <= irange; cur_step++) {
        vec2 tile = Transform::tile_infront_given_pos(pos, cur_step, direction);
        vec2 snapped_tile = vec::snap(tile);

        for (auto& entity_ref : matching) {
            Entity& entity = entity_ref.get();
            if (entity.is_missing<Transform>()) continue;

            vec2 entity_pos = vec::to2(entity.get<Transform>().snap_position());
            if (entity_pos == snapped_tile) {
                return entity;
            }
        }
    }
    return {};
}

OptEntity EQ::getMatchingEntityInFront(
    vec2 pos, Transform::FrontFaceDirection direction, float range,
    const std::function<bool(const Entity&)>& filter) {
    TRACY_ZONE_SCOPED;
    VALIDATE(range > 0,
             fmt::format("range has to be positive but was {}", range));
    /**
        auto& eq = EQ()
                   .whereLambda(filter)
                   .whereInFront(tile)
                   .whereSnappedPositionMatches(vec::snap(tile));
        if (eq.has_values()) {
            return eq.gen_first();
        }
     */

    int cur_step = 0;
    int irange = static_cast<int>(range);
    while (cur_step <= irange) {
        auto tile = Transform::tile_infront_given_pos(pos, cur_step, direction);

        for (const auto& current_entity : EntityHelper::get_entities()) {
            if (!current_entity) continue;
            if (!filter(*current_entity)) continue;

            // all entitites should have transforms but just in case
            if (current_entity->template is_missing<Transform>()) {
                log_warn("component {} is missing transform",
                         current_entity->id);
                log_error("component {} is missing name",
                          str(get_entity_type(*current_entity)));
                continue;
            }

            const Transform& transform =
                current_entity->template get<Transform>();

            float cur_dist = vec::distance(transform.as2(), tile);
            // outside reach
            if (abs(cur_dist) > 1) continue;
            // this is behind us
            if (cur_dist < 0) continue;

            // TODO :BE: add a snap_as2() function to transform
            if (vec::to2(transform.snap_position()) == vec::snap(tile)) {
                return *current_entity;
            }
        }
        cur_step++;
    }
    return {};
}
