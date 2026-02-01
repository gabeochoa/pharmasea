
#pragma once

#include "ah.h"
#include "components/can_be_held.h"
#include "components/can_hold_furniture.h"
#include "components/can_hold_item.h"
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

    // Explicit constructor for Entities (pharmasea's Entities type)
    explicit EQ(const ::Entities& entsIn)
        : afterhours::EntityQuery<EQ>(
              afterhours::Entities(entsIn.begin(), entsIn.end())) {}

    // Game-specific type filtering
    struct WhereType : EntityQuery::Modification {
        EntityType type;
        explicit WhereType(const EntityType& t) : type(t) {}
        bool operator()(const Entity& entity) const override {
            return entity.hasTag(type);
        }
    };
    EQ& whereType(const EntityType& t) { return add_mod(new WhereType(t)); }
    EQ& whereNotType(const EntityType& t) {
        return add_mod(new Not(new WhereType(t)));
    }

    // Range-based filtering
    struct WhereInRange : EntityQuery::Modification {
        vec2 position;
        float range;
        bool should_snap;

        explicit WhereInRange(vec2 pos, float r = 0.01f, bool snap = false)
            : position(pos), range(r), should_snap(snap) {}
        bool operator()(const Entity& entity) const override {
            vec2 pos = entity.get<Transform>().as2();
            if (should_snap) pos = vec::snap(pos);
            return vec::distance_sq(position, pos) < (range * range);
        }
    };
    EQ& whereInRange(vec2 position, float range) {
        return add_mod(new WhereInRange(position, range));
    }
    EQ& whereNotInRange(vec2 position, float range) {
        return add_mod(new Not(new WhereInRange(position, range)));
    }
    EQ& wherePositionMatches(const Entity& entity) {
        return whereInRange(entity.get<Transform>().as2(), 0.01f);
    }
    EQ& whereSnappedPositionMatches(vec2 position) {
        return add_mod(new WhereInRange(position, 0.01f, true));
    }
    EQ& whereSnappedPositionMatches(const Entity& entity) {
        return whereSnappedPositionMatches(entity.get<Transform>().as2());
    }

    // Direction-based filtering
    struct WhereInFront : EntityQuery::Modification {
        vec2 position;
        float range;

        explicit WhereInFront(vec2 pos, float r) : position(pos), range(r) {}
        bool operator()(const Entity& entity) const override {
            float dist = vec::distance(entity.get<Transform>().as2(), position);
            if (abs(dist) > range) return false;
            if (dist < 0) return false;
            return true;
        }
    };
    EQ& whereInFront(vec2 pos, float range = 1.f) {
        return add_mod(new WhereInFront(pos, range));
    }
    EQ& whereInFront(const Entity& entity, float range = 1.f) {
        return whereInFront(entity.get<Transform>().as2(), range);
    }

    // Bounding box filtering
    struct WhereInside : EntityQuery::Modification {
        vec2 min;
        vec2 max;

        explicit WhereInside(vec2 mn, vec2 mx) : min(mn), max(mx) {}

        bool operator()(const Entity& entity) const override {
            const auto pos = entity.get<Transform>().as2();
            if (pos.x > max.x || pos.x < min.x) return false;
            if (pos.y > max.y || pos.y < min.y) return false;
            return true;
        }
    };
    EQ& whereInside(vec2 range_min, vec2 range_max) {
        return add_mod(new WhereInside(range_min, range_max));
    }
    EQ& whereNotInside(vec2 range_min, vec2 range_max) {
        return add_mod(new Not(new WhereInside(range_min, range_max)));
    }

    // Collision filtering
    struct WhereCollides : EntityQuery::Modification {
        BoundingBox bounds;

        explicit WhereCollides(BoundingBox box) : bounds(box) {}

        bool operator()(const Entity& entity) const override {
            return CheckCollisionBoxes(entity.get<Transform>().bounds(),
                                       bounds);
        }
    };
    EQ& whereCollides(BoundingBox box) {
        return add_mod(new WhereCollides(box));
    }

    // Component + lambda filtering
    template<typename Component>
    struct WhereHasComponentAndLambda : EntityQuery::Modification {
        std::function<bool(const Component&)> fn;
        explicit WhereHasComponentAndLambda(
            const std::function<bool(const Component&)>& fn)
            : fn(fn) {}

        bool operator()(const Entity& entity) const override {
            return entity.has<Component>() && fn(entity.get<Component>());
        }
    };

    template<typename Component>
    EQ& whereHasComponentAndLambda(
        const std::function<bool(const Component&)>& fn) {
        return add_mod(new WhereHasComponentAndLambda<Component>(fn));
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

    // Complex query methods
    [[nodiscard]] OptEntity getClosestMatchingFurniture(
        const Transform& transform, float range,
        const std::function<bool(const Entity&)>& filter);

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
    struct WhereCanPathfindTo : EntityQuery::Modification {
        vec2 start;

        explicit WhereCanPathfindTo(vec2 starting_point)
            : start(starting_point) {}

        bool operator()(const Entity& entity) const override;
    };

    EQ& whereCanPathfindTo(const vec2& start) {
        return add_mod(new WhereCanPathfindTo(start));
    }

    // Held item filtering
    EQ& whereIsNotBeingHeld() {
        return add_mod(new WhereHasComponent<CanBeHeld>())
            .add_mod(new WhereLambda([](const Entity& entity) {
                return entity.get<CanBeHeld>().is_not_set();
            }));
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
            // Include store entities - no filter needed, just return
            return *this;
        } else {
            // Exclude store entities - filter out those with IsStoreSpawned
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
