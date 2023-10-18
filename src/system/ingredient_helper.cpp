
#include "ingredient_helper.h"

#include "../components/indexer.h"
#include "../engine/bitset_utils.h"
#include "../entity_helper.h"
#include "../recipe_library.h"

// TODO probably should move to the config json
std::vector<EntityTuple> IngredientHelper::get_machines_for_ingredient(
    Ingredient ig) {
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
                settings.push_back(EntityTuple{
                    .index = alc_index,
                    .igs = ets,
                });
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
                settings.push_back(EntityTuple{
                    .type = EntityTuple::type::And,
                    .igs = ets,
                });
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

std::map<Ingredient, std::vector<EntityTuple>>
IngredientHelper::get_machines_req_for_recipe(Drink drink) {
    std::map<Ingredient, std::vector<EntityTuple>> needed;

    bitset_utils::for_each_enabled_bit(
        get_req_ingredients_for_drink(drink), [&needed](size_t index) {
            Ingredient ig = magic_enum::enum_value<Ingredient>(index);
            needed[ig] = IngredientHelper::get_machines_for_ingredient(ig);
        });

    return needed;
}

bool IngredientHelper::has_machines_required_for_ingredient(
    const std::vector<RefEntity>& ents, Ingredient ig) {
    const auto doesAnyExist =
        [&ents](const std::function<bool(const Entity&)>& filter) {
            for (const Entity& e : ents) {
                if (filter(e)) return true;
            }
            return false;
        };

    const auto doesAnyExistWithType = [&doesAnyExist](EntityType type) {
        return doesAnyExist(
            [type](const Entity& entity) { return check_type(entity, type); });
    };

    switch (ig) {
        case Soda: {
            return doesAnyExistWithType(EntityType::SodaMachine);
        } break;
        case Rum:
        case Tequila:
        case Vodka:
        case Whiskey:
        case Gin:
        case TripleSec:
        case Bitters: {
            int alc_index = index_of<Ingredient, ingredient::Alcohols.size()>(
                ingredient::Alcohols, ig);
            return (
                // Alcohol Group
                doesAnyExistWithType(EntityType::MedicineCabinet) ||
                // Does a single one exist for this alcohol
                doesAnyExist([alc_index](const Entity& entity) {
                    if (!check_type(entity, EntityType::SingleAlcohol))
                        return false;
                    if (entity.is_missing<Indexer>()) return false;
                    return entity.get<Indexer>().value() == alc_index;
                })
                //
            );
        } break;
        case Pineapple:
        case Orange:
        case Coconut:
        case Cranberries:
        case Lime:
        case Lemon: {
            return doesAnyExistWithType(EntityType::PillDispenser);
        } break;
        case PinaJuice:
        case OrangeJuice:
        case CoconutCream:
        case CranberryJuice:
        case LimeJuice:
        case LemonJuice: {
            return doesAnyExistWithType(EntityType::PillDispenser) &&
                   doesAnyExistWithType(EntityType::Blender);
            return false;
        } break;
        case SimpleSyrup: {
            return doesAnyExistWithType(EntityType::SimpleSyrupHolder);
        } break;
        case IceCubes:
        case IceCrushed: {
            return doesAnyExistWithType(EntityType::IceMachine);
        } break;
        // TODO implement for these once thye have spawners
        case Salt:
        case MintLeaf:
        case Invalid:
            return false;
            break;
    }

    return true;
}

bool IngredientHelper::has_machines_required_for_ingredient(Ingredient ig) {
    return IngredientHelper::has_machines_required_for_ingredient(
        EntityHelper::get_ref_entities(), ig);
}
