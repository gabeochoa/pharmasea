

#include "../dataclass/ingredient.h"
#include "../engine/bitset_utils.h"
#include "../entity_helper.h"
#include "../recipe_library.h"
#include "../vendor_include.h"

struct EntityTuple {
    enum struct type { And, Or } type = type::Or;
    std::optional<int> index = {};
    std::vector<EntityType> igs;
};

inline std::vector<EntityTuple> get_machines_for_ingredient(Ingredient ig) {
    std::vector<EntityTuple> settings;
    switch (ig) {
        case Soda: {
            {
                std::vector<EntityType> ets;
                ets.push_back(EntityType::SodaMachine);
                settings.push_back(EntityTuple{.igs = ets});
            }
        } break;
        case Rum:
        case Tequila:
        case Vodka:
        case Whiskey:
        case Gin:
        case TripleSec:
        case Bitters: {
            {
                int alc_index =
                    index_of<Ingredient, ingredient::Alcohols.size()>(
                        ingredient::Alcohols, ig);
                std::vector<EntityType> ets;
                ets.push_back(EntityType::SingleAlcohol);
                settings.push_back(EntityTuple{.igs = ets, .index = alc_index});
            }
            {
                std::vector<EntityType> ets;
                ets.push_back(EntityType::MedicineCabinet);
                settings.push_back(EntityTuple{.igs = ets});
            }
        } break;
        case Pineapple:
        case Orange:
        case Coconut:
        case Cranberries:
        case Lime:
        case Lemon: {
            {
                std::vector<EntityType> ets;
                ets.push_back(EntityType::PillDispenser);
                settings.push_back(EntityTuple{.igs = ets});
            }
        } break;
        case PinaJuice:
        case OrangeJuice:
        case CoconutCream:
        case CranberryJuice:
        case LimeJuice:
        case LemonJuice: {
            {
                std::vector<EntityType> ets;
                ets.push_back(EntityType::Blender);
                ets.push_back(EntityType::PillDispenser);
                settings.push_back(
                    EntityTuple{.igs = ets, .type = EntityTuple::type::And});
            }
        } break;
        case SimpleSyrup: {
            {
                std::vector<EntityType> ets;
                ets.push_back(EntityType::SimpleSyrupHolder);
                settings.push_back(EntityTuple{.igs = ets});
            }
        } break;
        case IceCubes:
        case IceCrushed: {
            {
                std::vector<EntityType> ets;
                ets.push_back(EntityType::IceMachine);
                settings.push_back(EntityTuple{.igs = ets});
            }
        } break;
        // TODO implement for these once thye have spawners
        case Salt:
        case MintLeaf:
        case Invalid:
            break;
    }
    return settings;
}

inline std::map<Ingredient, std::vector<EntityTuple>>
get_machines_req_for_recipe(Drink drink) {
    std::map<Ingredient, std::vector<EntityTuple>> needed;

    bitset_utils::for_each_enabled_bit(
        get_req_ingredients_for_drink(drink), [&needed](size_t index) {
            Ingredient ig = magic_enum::enum_value<Ingredient>(index);
            needed[ig] = get_machines_for_ingredient(ig);
        });

    return needed;
}
