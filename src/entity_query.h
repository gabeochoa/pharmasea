
#pragma once

#include "entity.h"
#include "entity_helper.h"
#include "entity_type.h"

struct EntityQuery {
    struct Modification {
        virtual ~Modification() = default;
        virtual bool operator()(const Entity&) const = 0;
    };

    struct Not : Modification {
        std::unique_ptr<Modification> mod;

        explicit Not(Modification* m) : mod(m) {}

        bool operator()(const Entity& entity) const override {
            return !((*mod)(entity));
        }
    };

    struct Limit : Modification {
        int amount;
        mutable int amount_taken;
        explicit Limit(int amt) : amount(amt), amount_taken(0) {}

        bool operator()(const Entity&) const override {
            if (amount_taken > amount) return false;
            amount_taken++;
            return true;
        }
    };
    auto& take(int amount) { return add_mod(new Limit(amount)); }
    auto& first() { return take(1); }

    struct WhereID : Modification {
        int id;
        explicit WhereID(int id) : id(id) {}
        bool operator()(const Entity& entity) const override {
            return entity.id == id;
        }
    };
    auto& whereID(int id) { return add_mod(new WhereID(id)); }
    auto& whereNotID(int id) { return add_mod(new Not(new WhereID(id))); }

    struct WhereType : Modification {
        EntityType type;
        explicit WhereType(const EntityType& t) : type(t) {}
        bool operator()(const Entity& entity) const override {
            return check_type(entity, type);
        }
    };
    auto& whereType(const EntityType& t) { return add_mod(new WhereType(t)); }
    auto& whereNotType(const EntityType& t) {
        return add_mod(new Not(new WhereType(t)));
    }

    template<typename T>
    struct WhereHasComponent : Modification {
        bool operator()(const Entity& entity) const override {
            return entity.has<T>();
        }
    };
    template<typename T>
    auto& whereHasComponent() {
        return add_mod(new WhereHasComponent<T>());
    }
    template<typename T>
    auto& whereMissingComponent() {
        return add_mod(new Not(new WhereHasComponent<T>()));
    }

    struct WhereLambda : Modification {
        std::function<bool(const Entity&)> filter;
        explicit WhereLambda(const std::function<bool(const Entity&)>& cb)
            : filter(cb) {}
        bool operator()(const Entity& entity) const override {
            return filter(entity);
        }
    };
    auto& whereLambda(const std::function<bool(const Entity&)>& fn) {
        return add_mod(new WhereLambda(fn));
    }
    auto& whereLambdaExistsAndTrue(
        const std::function<bool(const Entity&)>& fn) {
        if (fn) return add_mod(new WhereLambda(fn));
        return *this;
    }

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
            return vec::distance(position, pos) < range;
        }
    };
    auto& whereInRange(vec2 position, float range) {
        return add_mod(new WhereInRange(position, range));
    }
    auto& whereNotInRange(vec2 position, float range) {
        return add_mod(new Not(new WhereInRange(position, range)));
    }
    auto& wherePositionMatches(const Entity& entity) {
        return whereInRange(entity.get<Transform>().as2(), 0.01f);
    }
    auto& whereSnappedPositionMatches(vec2 position) {
        // TODO mess around with the right epsilon here
        return add_mod(new WhereInRange(position, 0.01f, true));
    }
    auto& whereSnappedPositionMatches(const Entity& entity) {
        return whereSnappedPositionMatches(entity.get<Transform>().as2());
    }

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

    auto& whereInFront(vec2 pos, float range = 1.f) {
        return add_mod(new WhereInFront(pos, range));
    }

    auto& whereInFront(const Entity& entity, float range = 1.f) {
        return whereInFront(entity.get<Transform>().as2(), range);
    }

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
    auto& whereInside(vec2 range_min, vec2 range_max) {
        return add_mod(new WhereInside(range_min, range_max));
    }

    struct WhereCollides : Modification {
        BoundingBox bounds;

        explicit WhereCollides(BoundingBox box) : bounds(box) {}

        bool operator()(const Entity& entity) const override {
            return CheckCollisionBoxes(entity.get<Transform>().bounds(),
                                       bounds);
        }
    };
    auto& whereCollides(BoundingBox box) {
        return add_mod(new WhereCollides(box));
    }

    auto& whereIsNotBeingHeld() {
        return add_mod(new WhereHasComponent<CanBeHeld>())
            .add_mod(new WhereLambda([](const Entity& entity) {
                return entity.get<CanBeHeld>().is_not_held();
            }));
    }

    EntityQuery& whereIsHoldingFurnitureID(EntityID entityID);
    EntityQuery& whereIsHoldingItemOfType(EntityType type);
    EntityQuery& whereIsDrinkAndMatches(Drink recipe);

    EntityQuery& whereHeldItemMatches(
        const std::function<bool(const Entity&)>& fn);

    /////////

    using OrderByFn = std::function<bool(const Entity&, const Entity&)>;
    struct OrderBy {
        virtual ~OrderBy() {}
        virtual bool operator()(const Entity& a, const Entity& b) = 0;
    };

    struct OrderByLambda : OrderBy {
        OrderByFn sortFn;
        explicit OrderByLambda(const OrderByFn& sortFn) : sortFn(sortFn) {}

        bool operator()(const Entity& a, const Entity& b) override {
            return sortFn(a, b);
        }
    };

    auto& orderByLambda(const OrderByFn& sortfn) {
        return set_order_by(new OrderByLambda(sortfn));
    }

    auto& orderByDist(vec2 position) {
        return orderByLambda([=](const Entity& a, const Entity& b) {
            float a_dist = vec::distance(a.get<Transform>().as2(), position);
            float b_dist = vec::distance(b.get<Transform>().as2(), position);
            return a_dist < b_dist;
        });
    }

    /////////
    struct UnderlyingOptions {
        bool stop_on_first = false;
    };

    [[nodiscard]] bool has_values() const {
        return !run_query({.stop_on_first = true}).empty();
    }

    [[nodiscard]] bool is_empty() const {
        return run_query({.stop_on_first = true}).empty();
    }

    [[nodiscard]] RefEntities values_ignore_cache(
        UnderlyingOptions options) const {
        ents = run_query(options);
        return ents;
    }

    [[nodiscard]] RefEntities gen() const {
        if (!ran_query) return values_ignore_cache({});
        return ents;
    }

    [[nodiscard]] RefEntities gen_with_options(
        UnderlyingOptions options) const {
        if (!ran_query) return values_ignore_cache(options);
        return ents;
    }

    [[nodiscard]] OptEntity gen_first() const {
        if (has_values()) return (gen_with_options({.stop_on_first = true})[0]);
        return {};
    }

    [[nodiscard]] size_t gen_count() const {
        if (!ran_query) return values_ignore_cache({}).size();
        return ents.size();
    }

    [[nodiscard]] std::vector<int> gen_ids() const {
        const auto results = ran_query ? ents : values_ignore_cache({});
        std::vector<int> ids;
        for (const Entity& ent : results) {
            ids.push_back(ent.id);
        }
        return ids;
    }

    EntityQuery() : entities(EntityHelper::get_entities()) {}
    explicit EntityQuery(const Entities& ents) : entities(ents) {
        entities = ents;
    }

   private:
    Entities entities;

    std::unique_ptr<OrderBy> orderby;
    std::vector<std::unique_ptr<Modification>> mods;
    mutable RefEntities ents;
    mutable bool ran_query = false;

    EntityQuery& add_mod(Modification* mod) {
        mods.push_back(std::unique_ptr<Modification>(mod));
        return *this;
    }

    EntityQuery& set_order_by(OrderBy* ob) {
        if (orderby) {
            log_error(
                "We only apply the first order by in a query at the moment");
            return *this;
        }
        orderby = std::unique_ptr<OrderBy>(ob);
        return *this;
    }

    [[nodiscard]] RefEntities run_query(UnderlyingOptions options) const {
        RefEntities out;
        for (const auto& e_ptr : entities) {
            if (!e_ptr) continue;
            Entity& e = *e_ptr;

            bool passed_all_mods = std::ranges::all_of(
                mods, [&](const std::unique_ptr<Modification>& mod) -> bool {
                    return (*mod)(e);
                });

            if (passed_all_mods) out.push_back(e);
            if (options.stop_on_first && !out.empty()) return out;
        }

        // TODO :SPEED: if there is only one item no need to sort
        // TODO :SPEED: if we are doing gen_first() then partial sort?
        // Now run any order bys
        if (orderby) {
            std::sort(out.begin(), out.end(),
                      [&](const Entity& a, const Entity& b) {
                          return (*orderby)(a, b);
                      });
        }

        // TODO turn off cache for now
        // ran_query = true;
        return out;
    }
};
