
#pragma once

#include "entity.h"
#include "entity_helper.h"
#include "entity_type.h"

struct EntityQuery {
    struct Modification {
        virtual ~Modification() {}
        virtual bool operator()(const Entity&) const = 0;
    };

    struct Limit : Modification {
        int amount;
        mutable int amount_taken;
        explicit Limit(int amt) : amount(amt), amount_taken(0) {}

        virtual bool operator()(const Entity&) const override {
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
        virtual bool operator()(const Entity& entity) const override {
            return entity.id == id;
        }
    };
    auto& whereID(int id) { return add_mod(new WhereID(id)); }

    // TODO add predicates
    struct WhereNotID : Modification {
        int id;
        explicit WhereNotID(int id) : id(id) {}
        virtual bool operator()(const Entity& entity) const override {
            return entity.id != id;
        }
    };
    auto& whereNotID(int id) { return add_mod(new WhereNotID(id)); }

    struct WhereType : Modification {
        EntityType type;
        explicit WhereType(const EntityType& t) : type(t) {}
        virtual bool operator()(const Entity& entity) const override {
            return check_type(entity, type);
        }
    };
    auto& whereType(const EntityType& t) { return add_mod(new WhereType(t)); }

    template<typename T>
    struct WhereHasComponent : Modification {
        virtual bool operator()(const Entity& entity) const override {
            return entity.has<T>();
        }
    };
    template<typename T>
    auto& whereHasComponent() {
        return add_mod(new WhereHasComponent<T>());
    }

    struct WhereLambda : Modification {
        std::function<bool(const Entity&)> filter;
        explicit WhereLambda(const std::function<bool(const Entity&)>& cb)
            : filter(cb) {}
        virtual bool operator()(const Entity& entity) const override {
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
        virtual bool operator()(const Entity& entity) const override {
            vec2 pos = entity.get<Transform>().as2();
            if (should_snap) pos = vec::snap(pos);
            return vec::distance(position, pos) < range;
        }
    };
    auto& whereInRange(vec2 position, float range) {
        return add_mod(new WhereInRange(position, range));
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
        virtual bool operator()(const Entity& entity) const override {
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

        virtual bool operator()(const Entity& entity) const override {
            const auto pos = entity.get<Transform>().as2();
            if (pos.x > max.x || pos.x < min.x) return false;
            if (pos.y > max.y || pos.y < min.y) return false;
            return true;
        }
    };
    auto& whereInside(vec2 range_min, vec2 range_max) {
        return add_mod(new WhereInside(range_min, range_max));
    }
    /////////
    struct UnderlyingOptions {
        bool stop_on_first = false;
    };

    [[nodiscard]] bool has_values() const {
        return !run_query({.stop_on_first = true}).empty();
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

   private:
    std::vector<std::unique_ptr<Modification>> mods;
    mutable RefEntities ents;
    mutable bool ran_query = false;

    EntityQuery& add_mod(Modification* mod) {
        mods.push_back(std::unique_ptr<Modification>(mod));
        return *this;
    }

    [[nodiscard]] RefEntities run_query(UnderlyingOptions options) const {
        RefEntities out;
        for (const auto& e_ptr : EntityHelper::get_entities()) {
            if (!e_ptr) continue;
            Entity& e = *e_ptr;

            bool passed_all_mods = std::all_of(
                mods.begin(), mods.end(),
                [&](const std::unique_ptr<Modification>& mod) -> bool {
                    return (*mod)(e);
                });

            if (passed_all_mods) out.push_back(e);
            if (options.stop_on_first && !out.empty()) return out;
        }
        // TODO turn off cache for now
        // ran_query = true;
        return out;
    }
};
