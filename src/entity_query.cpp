
#include "entity_query.h"

//
#include "components/can_hold_furniture.h"
#include "components/can_hold_item.h"

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
