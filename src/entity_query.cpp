
#include "entity_query.h"

#include <memory>

//
#include "engine/pathfinder.h"

bool EntityQuery::WhereCanPathfindTo::operator()(const Entity& entity) const {
    return !pathfinder::find_path(
                start, entity.get<Transform>().tile_directly_infront(),
                std::bind(EntityHelper::isWalkable, std::placeholders::_1))
                .empty();
}

//
#include "components/can_hold_furniture.h"
#include "components/can_hold_item.h"

EntityQuery& EntityQuery::whereIsHoldingAnyFurniture() {
    return  //
        add_mod(new WhereHasComponent<CanHoldFurniture>())
            .add_mod(new WhereLambda([](const Entity& entity) {
                const CanHoldFurniture& chf = entity.get<CanHoldFurniture>();
                return chf.is_holding_furniture();
            }));
}

EntityQuery& EntityQuery::whereIsHoldingAnyFurnitureThatMatches(
    const std::function<bool(const Entity&)>& filter) {
    return  //
        add_mod(new WhereHasComponent<CanHoldFurniture>())
            .add_mod(new WhereLambda([&](const Entity& entity) {
                const CanHoldFurniture& chf = entity.get<CanHoldFurniture>();
                if (!chf.is_holding_furniture()) return false;
                OptEntity furniture =
                    EntityHelper::getEntityForID(chf.furniture_id());
                if (!furniture.has_value()) return false;

                return filter(furniture.asE());
            }));
}

EntityQuery& EntityQuery::whereIsHoldingFurnitureID(EntityID entityID) {
    return  //
        add_mod(new WhereHasComponent<CanHoldFurniture>())
            .add_mod(new WhereLambda([entityID](const Entity& entity) {
                const CanHoldFurniture& chf = entity.get<CanHoldFurniture>();
                return chf.is_holding_furniture() &&
                       chf.furniture_id() == entityID;
            }));
}

EntityQuery& EntityQuery::whereIsHoldingItemOfType(EntityType type) {
    return  //
        add_mod(new WhereHasComponent<CanHoldItem>())
            .add_mod(new WhereLambda([type](const Entity& entity) {
                const CanHoldItem& chi = entity.get<CanHoldItem>();
                return chi.is_holding_item() && chi.item().type == type;
            }));
}

EntityQuery& EntityQuery::whereIsDrinkAndMatches(Drink recipe) {
    return  //
        add_mod(new WhereHasComponent<IsDrink>())
            .add_mod(new WhereLambda([recipe](const Entity& entity) {
                return entity.get<IsDrink>().matches_drink(recipe);
            }));
}

EntityQuery& EntityQuery::whereHeldItemMatches(
    const std::function<bool(const Entity&)>& fn) {
    return  //
        add_mod(new WhereLambda([&fn](const Entity& entity) -> bool {
            const CanHoldItem& chf = entity.get<CanHoldItem>();
            if (!chf.is_holding_item()) return false;
            const Item& item = chf.item();
            return fn(item);
        }));
}

RefEntities EntityQuery::filter_mod(
    const RefEntities& in, const std::unique_ptr<Modification>& mod) const {
    RefEntities out;
    out.reserve(in.size());
    for (auto& entity : in) {
        if ((*mod)(entity)) {
            out.push_back(entity);
        }
    }
    return out;
}

RefEntities EntityQuery::run_query(UnderlyingOptions) const {
    RefEntities out;
    out.reserve(entities.size());

    for (const auto& e_ptr : entities) {
        if (!e_ptr) continue;
        Entity& e = *e_ptr;
        out.push_back(e);
    }

    // By default we want to ignore anything spawned in the store
    if (!_include_store_entities) {
        auto it = out.end();
        Modification* mod = new Not(new WhereHasComponent<IsStoreSpawned>());

        it = std::partition(out.begin(), it, [&mod](const auto& entity) {
            return (*mod)(entity);
        });

        delete mod;
    }

    auto it = out.end();
    for (auto& mod : mods) {
        it = std::partition(out.begin(), it, [&mod](const auto& entity) {
            return (*mod)(entity);
        });
    }

    out.erase(it, out.end());

    if (out.size() == 1) {
        return out;
    }

    // TODO :SPEED: if there is only one item no need to sort
    // TODO :SPEED: if we are doing gen_first() then partial sort?
    // Now run any order bys
    if (orderby) {
        std::sort(
            out.begin(), out.end(),
            [&](const Entity& a, const Entity& b) { return (*orderby)(a, b); });
    }

    // ran_query = true;
    return out;
}
