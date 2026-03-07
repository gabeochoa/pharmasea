
#pragma once

#include "ah.h"
#include "components/can_be_held.h"
#include "components/can_hold_furniture.h"
#include "components/can_hold_item.h"
#include "components/has_work.h"
#include "components/is_drink.h"
#include "components/is_store_spawned.h"
#include "components/transform.h"
#include "dataclass/ingredient.h"
#include "entity.h"
#include "entity_helper.h"
#include "entity_type.h"
#include "external_include.h"
#include "vec_util.h"

// Custom EntityQuery extending afterhours::EntityQuery with pharmasea-specific
// methods
struct EQ : public afterhours::EntityQuery<EQ> {
    EQ()
        // Default constructor uses current thread's EntityCollection
        : afterhours::EntityQuery<EQ>(EntityHelper::get_current_collection(),
                                      {.ignore_temp_warning = true}) {}

    // Constructor that accepts a specific EntityCollection
    explicit EQ(afterhours::EntityCollection& collection)
        : afterhours::EntityQuery<EQ>(collection,
                                      {.ignore_temp_warning = true}) {}

    // Both ::Entities and afterhours::Entities are
    // std::vector<std::shared_ptr<Entity>> so we pass directly by reference to
    // avoid creating a temporary (dangling ref).
    explicit EQ(const ::Entities& entsIn)
        : afterhours::EntityQuery<EQ>(entsIn) {}

    // Copy constructor - preserves accumulated modifications
    EQ(const EQ& other);

    // Game-specific type filtering
    EQ& whereType(const EntityType& t) {
        return add_filter([t](const Entity& e) { return e.hasTag(t); });
    }
    EQ& whereNotType(const EntityType& t) {
        return add_filter([t](const Entity& e) { return !e.hasTag(t); });
    }

    // Range-based filtering
    EQ& whereInRange(vec2 position, float range) {
        return add_filter([position, range](const Entity& e) {
            return vec::distance_sq(position, e.get<Transform>().as2()) <
                   (range * range);
        });
    }
    EQ& whereNotInRange(vec2 position, float range) {
        return add_filter([position, range](const Entity& e) {
            return vec::distance_sq(position, e.get<Transform>().as2()) >=
                   (range * range);
        });
    }
    EQ& wherePositionMatches(const Entity& entity) {
        return whereInRange(entity.get<Transform>().as2(), 0.01f);
    }
    EQ& whereSnappedPositionMatches(vec2 position) {
        return add_filter([position](const Entity& e) {
            vec2 pos = vec::snap(e.get<Transform>().as2());
            return vec::distance_sq(position, pos) < (0.01f * 0.01f);
        });
    }
    EQ& whereSnappedPositionMatches(const Entity& entity) {
        return whereSnappedPositionMatches(entity.get<Transform>().as2());
    }

    // Direction-based filtering
    EQ& whereInFront(vec2 pos, float range = 1.f) {
        return add_filter([pos, range](const Entity& e) {
            float dist = vec::distance(e.get<Transform>().as2(), pos);
            return abs(dist) <= range && dist >= 0;
        });
    }
    EQ& whereInFront(const Entity& entity, float range = 1.f) {
        return whereInFront(entity.get<Transform>().as2(), range);
    }

    // Bounding box filtering
    EQ& whereInside(vec2 range_min, vec2 range_max) {
        return add_filter([range_min, range_max](const Entity& e) {
            const auto pos = e.get<Transform>().as2();
            return pos.x <= range_max.x && pos.x >= range_min.x &&
                   pos.y <= range_max.y && pos.y >= range_min.y;
        });
    }
    EQ& whereNotInside(vec2 range_min, vec2 range_max) {
        return add_filter([range_min, range_max](const Entity& e) {
            const auto pos = e.get<Transform>().as2();
            return pos.x > range_max.x || pos.x < range_min.x ||
                   pos.y > range_max.y || pos.y < range_min.y;
        });
    }

    // Collision filtering
    EQ& whereCollides(BoundingBox box) {
        return add_filter([box](const Entity& e) {
            return CheckCollisionBoxes(e.get<Transform>().bounds(), box);
        });
    }

    // Component + lambda filtering
    template<typename Component>
    EQ& whereHasComponentAndLambda(
        const std::function<bool(const Component&)>& fn) {
        return add_filter([fn](const Entity& e) {
            return e.has<Component>() && fn(e.get<Component>());
        });
    }

    // Floor marker filtering
    EQ& whereFloorMarkerOfType(IsFloorMarker::Type type) {
        return whereHasComponentAndLambda<IsFloorMarker>(
            [type](const IsFloorMarker& fm) { return fm.type == type; });
    }

    // Trigger area filtering
    EQ& whereTriggerAreaOfType(IsTriggerArea::Type type) {
        return whereHasComponentAndLambda<IsTriggerArea>(
            [type](const IsTriggerArea& ta) { return ta.type == type; });
    }

    // Convenience filters for common patterns
    EQ& whereHasActiveWork() {
        return whereHasComponentAndLambda<HasWork>(
            [](const HasWork& hw) { return hw.has_work(); });
    }

    EQ& whereCanBePickedUp() {
        return whereHasComponentAndLambda<CanBeHeld>(
            [](const CanBeHeld& cbh) { return cbh.is_not_set(); });
    }

    EQ& whereIsHoldingItem() {
        return whereHasComponentAndLambda<CanHoldItem>(
            [](const CanHoldItem& chi) { return chi.is_holding_item(); });
    }

    EQ& whereHasEmptySlot() {
        return whereHasComponentAndLambda<CanHoldItem>(
            [](const CanHoldItem& chi) { return chi.empty(); });
    }

    // Terminal method: find closest entity in front matching accumulated
    // filters
    [[nodiscard]] OptEntity gen_closestInFront(const Transform& transform,
                                               float range);

    // Complex query methods
    [[nodiscard]] bool doesAnyExistWithType(const EntityType& type) {
        return whereType(type).has_values();
    }

    [[nodiscard]] OptEntity getMatchingEntityInFront(
        vec2 pos, Transform::FrontFaceDirection direction, float range,
        const std::function<bool(const Entity&)>& filter);

    [[nodiscard]] RefEntities getAllInRangeFiltered(
        vec2 range_min, vec2 range_max,
        const std::function<bool(const Entity&)>& filter) {
        return whereInside(range_min, range_max)
            .include_store_entities()
            .whereLambda(filter)
            .gen();
    }

    [[nodiscard]] RefEntities getAllInRange(vec2 range_min, vec2 range_max) {
        return whereInside(range_min, range_max).include_store_entities().gen();
    }

    [[nodiscard]] OptEntity getOverlappingEntityIfExists(
        const Entity& entity, float range,
        const std::function<bool(const Entity&)>& filter = {},
        bool include_store_entities = false) {
        const vec2 position = entity.get<Transform>().as2();
        return whereNotID(entity.id)
            .whereLambdaExistsAndTrue(filter)
            .whereHasComponent<IsSolid>()
            .whereInRange(position, range)
            .wherePositionMatches(entity)
            .include_store_entities(include_store_entities)
            .gen_first();
    }

    // Pathfinding filtering
    EQ& whereCanPathfindTo(const vec2& start);

    // Held item filtering
    EQ& whereIsNotBeingHeld() {
        return add_filter([](const Entity& e) {
            return e.has<CanBeHeld>() && e.get<CanBeHeld>().is_not_set();
        });
    }

    EQ& whereIsHoldingAnyFurniture();
    EQ& whereIsHoldingAnyFurnitureThatMatches(
        const std::function<bool(const Entity&)>&);
    EQ& whereIsHoldingFurnitureID(EntityID entityID);
    EQ& whereIsHoldingItemOfType(EntityType type);
    EQ& whereIsDrinkAndMatches(Drink recipe);
    EQ& whereHeldItemMatches(const std::function<bool(const Entity&)>& fn);

    // Store entity filtering
    // By default, store entities (with IsStoreSpawned) are excluded
    // Call include_store_entities() to include them, or
    // include_store_entities(false) to explicitly exclude
    EQ& include_store_entities(bool include = true) {
        if (include) {
            return *this;
        } else {
            return whereMissingComponent<IsStoreSpawned>();
        }
    }

    // Distance-based ordering
    EQ& orderByDist(vec2 position) {
        return orderByLambda([=](const Entity& a, const Entity& b) {
            float a_dist = vec::distance_sq(a.get<Transform>().as2(), position);
            float b_dist = vec::distance_sq(b.get<Transform>().as2(), position);
            return a_dist < b_dist;
        });
    }

    // Helper methods for extracting position data
    [[nodiscard]] std::optional<std::pair<int, vec3>> gen_first_position()
        const {
        if (!has_values()) return {};
        auto values = gen_with_options({.stop_on_first = true});
        if (values.empty()) return {};
        auto& ent = values[0].get();
        return std::pair{ent.id, ent.get<Transform>().pos()};
    }

    [[nodiscard]] std::vector<std::pair<EntityID, vec3>> gen_positions() const {
        const auto results = gen();
        std::vector<std::pair<EntityID, vec3>> ids;
        ids.reserve(results.size());
        std::transform(results.begin(), results.end(), std::back_inserter(ids),
                       [](const RefEntity& ent) -> std::pair<EntityID, vec3> {
                           return {ent.get().id,
                                   ent.get().get<Transform>().pos()};
                       });
        return ids;
    }
};

// Type alias - EQ is the new afterhours-based query, EntityQuery is kept for
// compatibility
using EntityQuery = EQ;
