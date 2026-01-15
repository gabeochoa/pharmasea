#include "save_file_load_fixers.h"

#include "components/has_dynamic_model_name.h"
#include "components/has_fishing_game.h"
#include "components/has_subtype.h"
#include "components/is_drink.h"
#include "components/is_item_container.h"
#include "dataclass/ingredient.h"
#include "entity.h"
#include "entity_helper.h"
#include "entity_type.h"
#include "post_deserialize_fixups.h"
#include "system/system_manager.h"
#include "util.h"

namespace server_only {

void fix_all_container_item_types(Entities& entities) {
    for (auto& entity : entities) {
        if (entity->has<IsItemContainer>()) {
            system_manager::fix_container_item_type(*entity);
        }
    }
}

void reinit_dynamic_model_names_after_load(Entities& entities) {
    // Loaded saves restore ECS data, but many components contain runtime-only
    // callbacks (std::function) that are not serialized. Recreate the dynamic
    // model-name fetchers here so we don't call empty std::function at runtime.
    for (const auto& sp : entities) {
        if (!sp) continue;
        Entity& e = *sp;

        if (e.has<HasDynamicModelName>()) {
            e.removeComponent<HasDynamicModelName>();
        }

        switch (static_cast<EntityType>(e.entity_type)) {
            case EntityType::Cupboard: {
                e.addComponent<HasDynamicModelName>().init(
                    EntityType::Cupboard,
                    HasDynamicModelName::DynamicType::OpenClosed);
            } break;
            case EntityType::Champagne: {
                e.addComponent<HasDynamicModelName>().init(
                    EntityType::Champagne,
                    HasDynamicModelName::DynamicType::Ingredients,
                    [](const Entity& owner, const std::string&) -> std::string {
                        return owner.get<HasFishingGame>().has_score()
                                   ? "champagne_open"
                                   : "champagne";
                    });
            } break;
            case EntityType::Alcohol: {
                e.addComponent<HasDynamicModelName>().init(
                    EntityType::Alcohol,
                    HasDynamicModelName::DynamicType::Subtype,
                    [](const Entity& owner, const std::string&) -> std::string {
                        const HasSubtype& hst = owner.get<HasSubtype>();
                        Ingredient bottle = get_ingredient_from_index(
                            (int) ingredient::AlcoholsInCycle[0] +
                            hst.get_type_index());
                        return util::toLowerCase(
                            magic_enum::enum_name<Ingredient>(bottle));
                    });
            } break;
            case EntityType::Fruit: {
                e.addComponent<HasDynamicModelName>().init(
                    EntityType::Fruit,
                    HasDynamicModelName::DynamicType::Subtype,
                    [](const Entity& owner, const std::string&) -> std::string {
                        const HasSubtype& hst = owner.get<HasSubtype>();
                        Ingredient fruit =
                            ingredient::Fruits[0 + hst.get_type_index()];
                        return util::convertToSnakeCase<Ingredient>(fruit);
                    });
            } break;
            case EntityType::Drink: {
                e.addComponent<HasDynamicModelName>().init(
                    EntityType::Drink,
                    HasDynamicModelName::DynamicType::Ingredients,
                    [](const Entity& owner, const std::string&) -> std::string {
                        const IsDrink& isdrink = owner.get<IsDrink>();
                        constexpr auto drinks =
                            magic_enum::enum_values<Drink>();
                        for (Drink d : drinks) {
                            if (isdrink.matches_drink(d)) {
                                return util::convertToSnakeCase<Drink>(d);
                            }
                        }
                        return "drink";
                    });
            } break;
            default: {
                // Most entities don't use HasDynamicModelName; nothing to do.
            } break;
        }
    }
}

void run_all_post_load_helpers() {
    // Always run fixups against the authoritative server collection.
    auto& collection = EntityHelper::get_server_collection();
    auto& entities = collection.entities_DO_NOT_USE;

    fix_all_container_item_types(entities);
    reinit_dynamic_model_names_after_load(entities);
    post_deserialize_fixups::run();
    // Add other post-load helpers here in the future
}

}  // namespace server_only
