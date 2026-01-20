
#pragma once

#include "../../dataclass/ingredient.h"
#include "../../entity.h"
#include "../../entity_type.h"
#include "../../vendor_include.h"

struct EntityTuple {
    enum struct type { And, Or } type = type::Or;
    std::optional<int> index = {};
    std::vector<EntityType> igs;
};

struct IngredientHelper {
    static std::vector<EntityTuple> get_machines_for_ingredient(Ingredient ig);
    static std::map<Ingredient, std::vector<EntityTuple>>
    get_machines_req_for_recipe(Drink drink);
    static bool has_machines_required_for_ingredient(Ingredient ig);
    static bool has_machines_required_for_ingredient(
        const std::vector<RefEntity>& ents, Ingredient ig);
};
