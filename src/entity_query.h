
#pragma once

#include <iterator>

#include "components/is_store_spawned.h"
#include "entity.h"
#include "entity_helper.h"
#include "entity_type.h"

struct EntityQuery : public afterhours::EntityQuery<EntityQuery> {
    using Base = afterhours::EntityQuery<EntityQuery>;
    using Modification = typename Base::Modification;
    using Not = typename Base::Not;
    template<typename T>
    using WhereHasComponent = typename Base::template WhereHasComponent<T>;
    using WhereLambda = typename Base::WhereLambda;
    using OrderByFn = typename Base::OrderByFn;
    using OrderBy = typename Base::OrderBy;
    using OrderByLambda = typename Base::OrderByLambda;

    EntityQuery() : Base(EntityHelper::get_entities()) {
        add_mod(new StoreEntityFilter(&_include_store_entities));
    }
    explicit EntityQuery(const Entities& ents) : Base(ents) {
        add_mod(new StoreEntityFilter(&_include_store_entities));
    }

    struct WhereType : Modification {
        EntityType type;
        explicit WhereType(const EntityType& t) : type(t) {}
        bool operator()(const Entity& entity) const override {
            return check_type(entity, type);
        }
    };
    auto& whereType(const EntityType& t) { return add_mod(new WhereType(t)); }
    auto& whereNotType(const EntityType& t) { return add_mod(new Not(new WhereType(t))); }

    struct WhereInRange : Modification {
        vec2 position;
        float range;
        bool should_snap;

        // TODO mess around with the right epsilon here
        explicit WhereInRange(vec2 pos, float r = 0.01f, bool snap = false)
            : position(pos), range(r), should_snap(snap) {}
        bool operator()(const Entity& entity) const override {
            vec2 pos = entity.get<Transform>().as2();
            if (should_snap) pos = vec::snap(pos);
            return vec::distance_sq(position, pos) < (range * range);
        }
    };
    auto& whereInRange(vec2 position, float range) { return add_mod(new WhereInRange(position, range)); }
    auto& whereNotInRange(vec2 position, float range) { return add_mod(new Not(new WhereInRange(position, range))); }
    auto& wherePositionMatches(const Entity& entity) { return whereInRange(entity.get<Transform>().as2(), 0.01f); }
    auto& whereSnappedPositionMatches(vec2 position) {
        // TODO mess around with the right epsilon here
        return add_mod(new WhereInRange(position, 0.01f, true));
    }
    auto& whereSnappedPositionMatches(const Entity& entity) { return whereSnappedPositionMatches(entity.get<Transform>().as2()); }

    struct WhereInFront : Modification {
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

    auto& whereInFront(vec2 pos, float range = 1.f) { return add_mod(new WhereInFront(pos, range)); }
    auto& whereInFront(const Entity& entity, float range = 1.f) { return whereInFront(entity.get<Transform>().as2(), range); }

    struct WhereInside : Modification {
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
    auto& whereInside(vec2 range_min, vec2 range_max) { return add_mod(new WhereInside(range_min, range_max)); }
    auto& whereNotInside(vec2 range_min, vec2 range_max) { return add_mod(new Not(new WhereInside(range_min, range_max))); }

    struct WhereCollides : Modification {
        BoundingBox bounds;

        explicit WhereCollides(BoundingBox box) : bounds(box) {}

        bool operator()(const Entity& entity) const override {
            return CheckCollisionBoxes(entity.get<Transform>().bounds(), bounds);
        }
    };
    auto& whereCollides(BoundingBox box) { return add_mod(new WhereCollides(box)); }

    auto& whereIsNotBeingHeld() {
        return add_mod(new WhereHasComponent<CanBeHeld>())
            .add_mod(new WhereLambda([](const Entity& entity) {
                return entity.get<CanBeHeld>().is_not_held();
            }));
    }

    EntityQuery& whereIsHoldingAnyFurniture();
    EntityQuery& whereIsHoldingAnyFurnitureThatMatches(const std::function<bool(const Entity&)>&);
    EntityQuery& whereIsHoldingFurnitureID(EntityID entityID);
    EntityQuery& whereIsHoldingItemOfType(EntityType type);
    EntityQuery& whereIsDrinkAndMatches(Drink recipe);

    EntityQuery& whereHeldItemMatches(const std::function<bool(const Entity&)>& fn);

    template<typename Component>
    struct WhereHasComponentAndLambda : Modification {
        std::function<bool(const Component&)> fn;
        explicit WhereHasComponentAndLambda(const std::function<bool(const Component&)>& fn) : fn(fn) {}

        bool operator()(const Entity& entity) const override {
            return entity.has<Component>() && fn(entity.get<Component>());
        }
    };

    template<typename Component>
    auto& whereHasComponentAndLambda(const std::function<bool(const Component&)>& fn) {
        return add_mod(new WhereHasComponentAndLambda<Component>(fn));
    }

    struct WhereCanPathfindTo : Modification {
        vec2 start;

        explicit WhereCanPathfindTo(vec2 starting_point) : start(starting_point) {}

        bool operator()(const Entity& entity) const override;
    };

    auto& whereCanPathfindTo(const vec2& start) { return add_mod(new WhereCanPathfindTo(start)); }

    // ordering helpers
    auto& orderByLambda(const OrderByFn& sortfn) { return Base::orderByLambda(sortfn); }
    auto& orderByDist(vec2 position) {
        return orderByLambda([=](const Entity& a, const Entity& b) {
            float a_dist = vec::distance_sq(a.get<Transform>().as2(), position);
            float b_dist = vec::distance_sq(b.get<Transform>().as2(), position);
            return a_dist < b_dist;
        });
    }

    // extra convenience generators
    [[nodiscard]] std::optional<std::pair<int, vec3>> gen_first_position() const {
        if (!has_values()) return {};
        auto ent_ = gen_with_options({.stop_on_first = true})[0];
        auto& ent = ent_.get();
        return std::pair{ent.id, ent.get<Transform>().pos()};
    }

    [[nodiscard]] std::vector<std::pair<EntityID, vec3>> gen_positions() const {
        const auto results = gen();
        std::vector<std::pair<EntityID, vec3>> ids;
        std::transform(results.begin(), results.end(), std::back_inserter(ids),
                   [](const Entity& ent) -> std::pair<EntityID, vec3> {
                   return {ent.id, ent.get<Transform>().pos()};
               });
        return ids;
    }

    auto& include_store_entities(bool include = true) {
        _include_store_entities = include;
        return *this;
    }

   private:
    struct StoreEntityFilter : Modification {
        const bool* include_store_entities_flag;
        explicit StoreEntityFilter(const bool* flag) : include_store_entities_flag(flag) {}
        bool operator()(const Entity& entity) const override {
            if (*include_store_entities_flag) return true;
            return entity.is_missing<IsStoreSpawned>();
        }
    };

    bool _include_store_entities = false;
};
