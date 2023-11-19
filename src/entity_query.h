
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
            amount_taken++;
            return amount_taken < amount;
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

    struct WhereType : Modification {
        EntityType type;
        explicit WhereType(const EntityType& t) : type(t) {}
        virtual bool operator()(const Entity& entity) const override {
            return check_type(entity, type);
        }
    };
    auto& whereType(const EntityType& t) { return add_mod(new WhereType(t)); }

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

    struct WhereInRange : Modification {
        vec2 position;
        float range;

        explicit WhereInRange(vec2 pos, float r) : position(pos), range(r) {}
        virtual bool operator()(const Entity& entity) const override {
            return vec::distance(position, entity.get<Transform>().as2()) <
                   range;
        }
    };
    auto& whereInRange(vec2 position, float range) {
        return add_mod(new WhereInRange(position, range));
    }

    /////////

    [[nodiscard]] bool has_values() const { return !run_query(true).empty(); }

    [[nodiscard]] RefEntities values_ignore_cache() const {
        ents = run_query();
        return ents;
    }

    [[nodiscard]] RefEntities gen() const {
        if (!ran_query) return values_ignore_cache();
        return ents;
    }

    [[nodiscard]] OptEntity gen_first() const {
        if (has_values()) return (gen()[0]);
        return {};
    }

   private:
    std::vector<std::unique_ptr<Modification>> mods;
    mutable RefEntities ents;
    mutable bool ran_query = false;

    EntityQuery& add_mod(Modification* mod) {
        mods.push_back(std::unique_ptr<Modification>(mod));
        return *this;
    }

    [[nodiscard]] RefEntities run_query(bool stop_on_first = false) const {
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
            if (stop_on_first && !out.empty()) return out;
        }
        // ran_query = true;
        return out;
    }
};
