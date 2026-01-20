// System includes - each system is in its own header file to improve build
// times
#include "count_all_possible_trigger_area_entrants_system.h"
#include "count_in_building_trigger_area_entrants_system.h"
#include "count_trigger_area_entrants_system.h"
#include "mark_trigger_area_full_needs_processing_system.h"
#include "reset_trigger_fired_while_occupied_system.h"
#include "trigger_cb_on_full_progress_system.h"
#include "update_dynamic_trigger_area_settings_system.h"
#include "update_trigger_area_percent_system.h"

// Additional includes for store functions
#include "../../ah.h"
#include "../../components/can_hold_item.h"
#include "../../components/custom_item_position.h"
#include "../../components/is_bank.h"
#include "../../components/is_floor_marker.h"
#include "../../components/is_free_in_store.h"
#include "../../components/is_progression_manager.h"
#include "../../components/is_round_settings_manager.h"
#include "../../components/is_store_spawned.h"
#include "../../components/transform.h"
#include "../../dataclass/ingredient.h"
#include "../../engine/log.h"
#include "../../entity.h"
#include "../../entity_helper.h"
#include "../../entity_id.h"
#include "../../entity_makers.h"
#include "../../entity_query.h"
#include "../../entity_type.h"
#include "../../system/helpers/ingredient_helper.h"
#include "../../system/sixtyfps/afterhours_held_position_helpers.h"

namespace system_manager {

// Global state for load/save delete mode
bool g_load_save_delete_mode = false;

namespace store {
void move_purchased_furniture() {
    // Grab the overlap area so we can see what it marked
    OptEntity purchase_area =
        EQ().whereFloorMarkerOfType(IsFloorMarker::Type::Store_PurchaseArea)
            .gen_first();
    const IsFloorMarker& ifm = purchase_area->get<IsFloorMarker>();

    // Grab the plannig spawn area so we can place in the right spot
    OptEntity spawn_area =
        EQ().whereFloorMarkerOfType(IsFloorMarker::Type::Planning_SpawnArea)
            .gen_first();
    vec3 spawn_position = spawn_area->get<Transform>().pos();

    OptEntity sophie = EntityQuery().whereType(EntityType::Sophie).gen_first();
    VALIDATE(sophie.valid(), "sophie should exist when moving furniture");
    IsBank& bank = sophie->get<IsBank>();

    int amount_in_cart = 0;

    // for every marked, move them over
    for (size_t i = 0; i < ifm.num_marked(); i++) {
        EntityID id = ifm.marked_ids()[i];
        OptEntity marked_entity = EntityHelper::getEntityForID(id);
        if (!marked_entity) continue;

        // Move it to the new spot
        Transform& transform = marked_entity->get<Transform>();
        transform.update(spawn_position);
        transform.update_y(0);

        // Remove the 'store_cleanup' marker
        if (marked_entity->has<IsStoreSpawned>()) {
            marked_entity->removeComponent<IsStoreSpawned>();
        }

        // Some items can hold other items; move the held item with the
        // furniture
        if (marked_entity->has<CanHoldItem>()) {
            CanHoldItem& chi = marked_entity->get<CanHoldItem>();
            if (!chi.empty()) {
                vec3 new_pos =
                    marked_entity->has<CustomHeldItemPosition>()
                        ? get_new_held_position_custom(marked_entity.asE())
                        : get_new_held_position_default(marked_entity.asE());
                OptEntity held_opt = chi.item();
                if (held_opt) {
                    held_opt.asE().get<Transform>().update(new_pos);
                } else {
                    chi.update(nullptr, marked_entity->id);
                }
            }
        }

        // Its not free!
        if (marked_entity->is_missing<IsFreeInStore>()) {
            amount_in_cart +=
                std::max(0, get_price_for_entity_type(
                                get_entity_type(marked_entity.asE())));
        }
    }

    VALIDATE(amount_in_cart == bank.cart(),
             "Amount we computed for in cart should always match the actual "
             "amount in our cart")

    // Commit the withdraw
    bank.withdraw(amount_in_cart);
    bank.update_cart(0);
}
}  // namespace store

void spawn_machines_for_newly_unlocked_drink_DONOTCALL(
    IsProgressionManager& ipm, Drink option) {
    // today we dont have a way to statically know which machines
    // provide which ingredients because they are dynamic
    IngredientBitSet possibleNewIGs = get_req_ingredients_for_drink(option);

    // Because prereqs are handled, we dont need do check for them and can
    // assume that those bits will handle checking for it

    OptEntity spawn_area =
        EQ().whereFloorMarkerOfType(
                // Note we spawn free items in the purchase area so its more
                // obvious that they are free
                IsFloorMarker::Type::Planning_SpawnArea)
            .gen_first();

    if (!spawn_area) {
        // need to guarantee this exists long before we get here
        log_error("Could not find spawn area entity");
    }

    bitset_utils::for_each_enabled_bit(possibleNewIGs, [&ipm, spawn_area](
                                                           size_t index) {
        Ingredient ig = magic_enum::enum_value<Ingredient>(index);

        const auto make_free_machine = []() -> Entity& {
            auto& entity = EntityHelper::createEntity();
            entity.addComponent<IsFreeInStore>();
            return entity;
        };

        // Already has the machine so we good
        if (IngredientHelper::has_machines_required_for_ingredient(ig))
            return bitset_utils::ForEachFlow::Continue;

        switch (ig) {
            case Soda: {
                // Nothing is needed to do for this since
                // the soda machine is required to play the game
            } break;
            case Beer: {
                auto et = EntityType::DraftTap;
                auto& entity = make_free_machine();
                convert_to_type(et, entity, spawn_area->get<Transform>().as2());
                ipm.unlock_entity(et);
            } break;
            case Champagne: {
                auto et = EntityType::ChampagneHolder;
                auto& entity = make_free_machine();
                convert_to_type(et, entity, spawn_area->get<Transform>().as2());
                ipm.unlock_entity(et);
            } break;
            case Rum:
            case Tequila:
            case Vodka:
            case Whiskey:
            case Gin:
            case TripleSec:
            case Bitters: {
                int alc_index =
                    index_of<Ingredient, ingredient::AlcoholsInCycle.size()>(
                        ingredient::AlcoholsInCycle, ig);
                auto& entity = make_free_machine();
                furniture::make_single_alcohol(
                    entity, spawn_area->get<Transform>().as2(), alc_index);
            } break;
            case Pineapple:
            case Orange:
            case Coconut:
            case Cranberries:
            case Lime:
            case Lemon: {
                auto et = EntityType::FruitBasket;
                auto& entity = make_free_machine();
                convert_to_type(et, entity, spawn_area->get<Transform>().as2());
                ipm.unlock_entity(et);
            } break;
            case PinaJuice:
            case OrangeJuice:
            case CoconutCream:
            case CranberryJuice:
            case LimeJuice:
            case LemonJuice: {
                auto et = EntityType::Blender;
                auto& entity = make_free_machine();
                convert_to_type(et, entity, spawn_area->get<Transform>().as2());

                ipm.unlock_entity(et);
            } break;
            case SimpleSyrup: {
                auto et = EntityType::SimpleSyrupHolder;
                auto& entity = make_free_machine();
                convert_to_type(et, entity, spawn_area->get<Transform>().as2());
                ipm.unlock_entity(et);
            } break;
            case IceCubes:
            case IceCrushed: {
                auto et = EntityType::IceMachine;
                auto& entity = make_free_machine();
                convert_to_type(et, entity, spawn_area->get<Transform>().as2());
                ipm.unlock_entity(et);
            } break;
            case Salt:
            case MintLeaf:
                // TODO implement for these once thye have spawners
                break;
            case Invalid:
                break;
        }
        return bitset_utils::ForEachFlow::NormalFlow;
    });
}

void generate_machines_for_new_upgrades() {
    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

    OptEntity purchase_area =
        EQ().whereFloorMarkerOfType(IsFloorMarker::Type::Planning_SpawnArea)
            .gen_first();

    for (EntityType et : irsm.config.store_to_spawn) {
        auto& entity = EntityHelper::createEntity();
        bool success =
            convert_to_type(et, entity, purchase_area->get<Transform>().as2());
        if (!success) {
            entity.cleanup = true;
            log_error(
                "Store spawn of newly unlocked item failed to "
                "generate");
        }
    }
    irsm.config.store_to_spawn.clear();
}

void register_trigger_area_systems(afterhours::SystemManager& systems) {
    systems.register_update_system(
        std::make_unique<
            system_manager::UpdateDynamicTriggerAreaSettingsSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::CountAllPossibleTriggerAreaEntrantsSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::CountInBuildingTriggerAreaEntrantsSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::CountTriggerAreaEntrantsSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::UpdateTriggerAreaPercentSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::ResetTriggerFiredWhileOccupiedSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::MarkTriggerAreaFullNeedsProcessingSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::TriggerCbOnFullProgressSystem>());
}

}  // namespace system_manager
