

#pragma once

#include <random>

#include "../ah.h"
#include "../engine/random_engine.h"
//
#include "../dataclass/ingredient.h"
#include "../dataclass/settings.h"
#include "../dataclass/upgrade_class.h"
#include "../recipe_library.h"
#include "../strings.h"
#include "base_component.h"

struct IsProgressionManager : public BaseComponent {
    IsProgressionManager() {}

    void init() {
        unlock_drink(Drink::coke);
        unlock_ingredient(Ingredient::Soda);

        // Unlock all the starting store items
        magic_enum::enum_for_each<EntityType>([&](EntityType val) {
            StoreEligibilityType set = get_store_eligibility(val);
            if (set == StoreEligibilityType::OnStart ||
                set == StoreEligibilityType::TimeBased) {
                unlock_entity(val);
            }
        });

        log_trace("create: {} {}", enabledDrinks, enabledIngredients);
    }

    IsProgressionManager& unlock_drink(const Drink& drink) {
        log_trace("unlocking drink {}", magic_enum::enum_name<Drink>(drink));
        enabledDrinks.set(drink);
        log_trace("unlock drink: {} {}", enabledDrinks, enabledIngredients);
        lastUnlockedDrink = drink;

        // Sometimes we might unlock something (through upgrade probably)
        // where we didnt unlock the ingredients for yet
        // this is probably an oversight so we will do it but complain
        if (!can_create_drink(drink)) {
            IngredientBitSet ings = get_recipe_for_drink(drink);
            IngredientBitSet overlap = ings & enabledIngredients;
            bitset_utils::for_each_enabled_bit(overlap, [&](size_t index) {
                Ingredient ig = magic_enum::enum_value<Ingredient>(index);

                log_warn(
                    "You are unlocking drink {} but were missing unlocking "
                    "{}",
                    magic_enum::enum_name<Drink>(drink),
                    magic_enum::enum_name<Ingredient>(ig));

                unlock_ingredient(ig);
                return bitset_utils::ForEachFlow::NormalFlow;
            });
        }

        return *this;
    }

    IsProgressionManager& unlock_ingredient(const Ingredient& ing) {
        log_trace("unlocking ingredient {}",
                  magic_enum::enum_name<Ingredient>(ing));
        enabledIngredients.set(ing);
        log_trace("unlock ingredient: {} {}", enabledDrinks,
                  enabledIngredients);
        return *this;
    }

    IsProgressionManager& unlock_entity(const EntityType& etype) {
        if (get_store_eligibility(etype) == StoreEligibilityType::Never) {
            log_warn("You are unlocking {} but its been marked never unlock",
                     etype);
        }
        log_trace("unlocking entity{}",
                  magic_enum::enum_name<EntityType>(etype));
        const auto index = magic_enum::enum_index<EntityType>(etype).value();
        unlockedEntityTypes.set(index);
        return *this;
    }

    [[nodiscard]] DrinkSet enabled_drinks() const { return enabledDrinks; }
    [[nodiscard]] IngredientBitSet enabled_ingredients() const {
        return enabledIngredients;
    }
    [[nodiscard]] EntityTypeSet enabled_entity_types() const {
        return unlockedEntityTypes;
    }

    Drink get_random_unlocked_drink() const {
        log_trace("get random: {} {}", enabledDrinks, enabledIngredients);
        int drinkSetBit = bitset_utils::get_random_enabled_bit(
            enabledDrinks, RandomEngine::rng());
        if (drinkSetBit == -1) {
            log_warn("generated {} but we had {} enabled drinks", drinkSetBit,
                     enabledDrinks.count());
            return Drink::coke;
        }
        return magic_enum::enum_cast<Drink>(drinkSetBit).value();
    }

    // checks the current enabled ingredients to see if its possible to
    // create the drink
    bool can_create_drink(Drink drink) const {
        IngredientBitSet ings = get_recipe_for_drink(drink);

        IngredientBitSet overlap = ings & enabledIngredients;
        return overlap == ings;
    }

    [[nodiscard]] bool is_drink_unlocked(Drink drink) const {
        size_t index = magic_enum::enum_index<Drink>(drink).value();
        return enabledDrinks.test(index);
    }

    [[nodiscard]] bool is_ingredient_unlocked(Ingredient ingredient) const {
        size_t index = magic_enum::enum_index<Ingredient>(ingredient).value();
        return enabledIngredients.test(index);
    }

    [[nodiscard]] bool is_ingredient_locked(Ingredient ingredient) const {
        return !is_ingredient_unlocked(ingredient);
    }

    [[nodiscard]] Drink get_last_unlocked() const { return lastUnlockedDrink; }

    [[nodiscard]] bool is_entity_unlocked(EntityType etype) const {
        size_t index = magic_enum::enum_index<EntityType>(etype).value();
        return unlockedEntityTypes.test(index);
    }

    int upgrade_index = 0;

    [[nodiscard]] UpgradeType upgrade_type() const {
        return upgrade_rounds[upgrade_index];
    }

    [[nodiscard]] TranslatableString get_option_title(bool is_first) const;

    [[nodiscard]] TranslatableString get_option_subtitle(bool is_first) const;

    void next_round() {
        // Increment to the next upgrade type (drink -> upgrade etc)
        upgrade_index = (upgrade_index + 1) % upgrade_rounds.size();
        collectedOptions = false;
    }

    // TODO make private
    bool collectedOptions = false;

    Drink drinkOption1 = coke;
    Drink drinkOption2 = coke;

    UpgradeClass upgradeOption1;
    UpgradeClass upgradeOption2;

    Drink lastUnlockedDrink = coke;

    DrinkSet enabledDrinks;
    IngredientBitSet enabledIngredients;
    EntityTypeSet unlockedEntityTypes;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.ext(enabledDrinks, bitsery::ext::StdBitset{});
        s.ext(enabledIngredients, bitsery::ext::StdBitset{});
        s.ext(unlockedEntityTypes, bitsery::ext::StdBitset{});

        s.value4b(drinkOption1);
        s.value4b(drinkOption2);

        s.value4b(upgradeOption1);
        s.value4b(upgradeOption2);

        s.value4b(upgrade_index);
        s.value1b(collectedOptions);
    }
};
