#include "ai_system.h"

#include "../components/can_hold_item.h"
#include "../components/can_order_drink.h"
#include "../components/has_base_speed.h"
#include "../components/is_round_settings_manager.h"
#include "../engine/runtime_globals.h"
#include "../entity_helper.h"
#include "../recipe_library.h"

namespace system_manager::ai {

namespace {
[[nodiscard]] bool needs_bathroom_now_internal(Entity& entity) {
    if (entity.is_missing<CanOrderDrink>()) return false;
    if (entity.is_missing<IsAIControlled>()) return false;
    if (!entity.get<IsAIControlled>().has_ability(
            IsAIControlled::AbilityUseBathroom))
        return false;

    // Don't send them to the bathroom while holding something.
    if (entity.has<CanHoldItem>()) {
        if (!entity.get<CanHoldItem>().empty()) return false;
    }

    OptEntity sophie_opt =
        EntityHelper::getPossibleNamedEntity(NamedEntity::Sophie);
    if (!sophie_opt) return false;
    Entity& sophie = sophie_opt.asE();
    if (sophie.is_missing<IsRoundSettingsManager>()) return false;
    const IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();
    int bladder_size = irsm.get<int>(ConfigKey::BladderSize);
    const CanOrderDrink& cod = entity.get<CanOrderDrink>();
    return cod.get_drinks_in_bladder() >= bladder_size;
}
}  // namespace

[[nodiscard]] bool needs_bathroom_now(Entity& entity) {
    return needs_bathroom_now_internal(entity);
}

bool validate_drink_order(const Entity& customer, Drink orderedDrink,
                          Item& madeDrink) {
    const Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    const IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();
    // TODO :DESIGN: how many ingredients have to be correct?
    // as people get more drunk they should care less and less
    //
    bool all_ingredients_match =
        madeDrink.get<IsDrink>().matches_drink(orderedDrink);

    // all good
    if (all_ingredients_match) return true;
    // Otherwise Something was wrong with the drink

    // For debug, if we have this set, just assume it was correct
    if (globals::skip_ingredient_match()) {
        return true;
    }

    // If you have the mocktail upgrade an an ingredient was wrong,
    // figure out if it was alcohol and if theres only one missing then
    // we are good
    if (irsm.has_upgrade_unlocked(UpgradeClass::Mocktails)) {
        Recipe recipe = RecipeLibrary::get().get(
            std::string(magic_enum::enum_name(orderedDrink)));

        IngredientBitSet orderedDrinkSet = recipe.ingredients;
        IngredientBitSet madeDrinkSet = madeDrink.get<IsDrink>().ing();

        // Imagine we have a Mojito order (Soda, Rum, LimeJ, SimpleSyrup for
        // ex) We also support Whiskey. So mojito would look like 11011
        // (whiskey being zero)
        //
        // Two examples, one where we forget the rum (good) and one where we
        // swap rum with whiskey (bad)
        //
        // one, madeDrink 10011 => this is good
        // 11011 xor 10011 => 01000
        // count_bits(01000) => 1 (good)
        // is_bit_alc(01000) => true (good)
        // return true;
        //
        // two madeDrink 10111 => this is bad
        // 11011 xor 10111 => 01100
        // count_bits(01100) => 2 (bad)
        // is_bit_alc(01100) => true for both (?mixed)

        auto xorbits = orderedDrinkSet ^ madeDrinkSet;
        // How many ingredients did we mess up?
        if (xorbits.count() != 1) {
            // TODO this return is what keeps us from being able to support
            // both upgrades at the same time (if we wanted that)
            return false;
        }
        // TODO idk if index is right 100% of the time but lets try it
        Ingredient ig = get_ingredient_from_index(
            bitset_utils::get_first_enabled_bit(xorbits));

        // is the (one) ingredient we messed up an alcoholic one?
        // if so then we are good
        if (ingredient::is_alcohol(ig)) {
            return true;
        }
    }

    if (irsm.has_upgrade_unlocked(UpgradeClass::CantEvenTell)) {
        const CanOrderDrink& cod = customer.get<CanOrderDrink>();
        size_t num_alc_drank = cod.num_alcoholic_drinks_drank();

        Recipe recipe = RecipeLibrary::get().get(
            std::string(magic_enum::enum_name(orderedDrink)));
        IngredientBitSet orderedDrinkSet = recipe.ingredients;
        IngredientBitSet madeDrinkSet = madeDrink.get<IsDrink>().ing();

        auto xorbits = orderedDrinkSet ^ madeDrinkSet;
        size_t num_messed_up = xorbits.count();

        // You messed up less ingredients than
        // the number of drinks they had
        //
        // So they cant tell :)
        //
        if (num_messed_up < num_alc_drank) {
            return true;
        }
    }

    return false;
}

float get_speed_for_entity(Entity& entity) {
    float base_speed = entity.get<HasBaseSpeed>().speed();

    // TODO Does OrderDrink hold stagger information?
    // or should it live in another component?
    if (entity.has<CanOrderDrink>()) {
        const CanOrderDrink& cha = entity.get<CanOrderDrink>();
        // float speed_multiplier = cha.ailment().speed_multiplier();
        // if (speed_multiplier != 0) base_speed *= speed_multiplier;

        // TODO Turning off stagger; couple problems
        // - configuration is hard to reason about and mess with
        // - i really want it to cause them to move more, maybe we place
        // this in the path generation or something isntead?
        //
        // float stagger_multiplier = cha.ailment().stagger(); if
        // (stagger_multiplier != 0) base_speed *= stagger_multiplier;

        int denom = RandomEngine::get().get_int(
            1, std::max(1, cha.num_alcoholic_drinks_drank()));
        base_speed *= 1.f / denom;

        base_speed = fmaxf(1.f, base_speed);
        // log_info("multiplier {} {} {}", speed_multiplier,
        // stagger_multiplier, base_speed);
    }
    return base_speed;
}

}  // namespace system_manager::ai
