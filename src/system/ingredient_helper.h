

#include "../dataclass/ingredient.h"
#include "../entity_type.h"
#include "../vendor_include.h"

struct EntityTuple {
    enum struct type { And, Or } type = type::Or;
    std::optional<int> index = {};
    std::vector<EntityType> igs;
};

std::vector<EntityTuple> get_machines_for_ingredient(Ingredient ig);
std::map<Ingredient, std::vector<EntityTuple>> get_machines_req_for_recipe(
    Drink drink);
bool has_machines_required_for_ingredient(Ingredient ig);
