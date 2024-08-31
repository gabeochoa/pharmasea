
#include "recipe_library.h"

#include "components/is_round_settings_manager.h"
#include "vendor_include.h"
//
#include "components/is_progression_manager.h"
#include "entity_query.h"

// TODO at some point these should be one path instead of separate
// so we can avoid weirdness
//
int get_average_unlocked_drink_cost() {
    OptEntity opt_sophie =
        EntityQuery().whereHasComponent<IsProgressionManager>().gen_first();
    if (!opt_sophie.has_value()) return 0;

    Entity& sophie = opt_sophie.asE();

    const IsProgressionManager& progressionManager =
        sophie.get<IsProgressionManager>();
    const IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

    DrinkSet unlockedDrinks = progressionManager.enabled_drinks();

    float total_price = 0;
    int num_drinks = (int) unlockedDrinks.count();

    bitset_utils::for_each_enabled_bit(unlockedDrinks, [&](size_t index) {
        Drink drink = magic_enum::enum_value<Drink>(index);

        total_price += get_base_price_for_drink(drink);

        return bitset_utils::ForEachFlow::NormalFlow;
    });

    // TODO this doesnt work because irsm config isnt serialized....
    // because its a map of variant, its not really that easy to do
    float cost_multiplier = irsm.get<float>(ConfigKey::DrinkCostMultiplier);
    total_price *= cost_multiplier;

    return (int) (total_price / num_drinks);
}

std::tuple<int, int> get_price_for_order(OrderInfo order_info) {
    OptEntity opt_sophie =
        EntityQuery().whereHasComponent<IsRoundSettingsManager>().gen_first();
    if (!opt_sophie.has_value()) return {0, 0};

    Entity& sophie = opt_sophie.asE();
    IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

    // mark how much we are paying for this drink
    // + how much we will tip
    float base_price = get_base_price_for_drink(order_info.order);

    float speakeasy_multiplier = 1.f;
    if (irsm.has_upgrade_unlocked(UpgradeClass::Speakeasy)) {
        speakeasy_multiplier += (0.01f * order_info.max_pathfind_distance);
    }

    float cost_multiplier = irsm.get<float>(ConfigKey::DrinkCostMultiplier);

    float price_float = cost_multiplier * speakeasy_multiplier * base_price;
    int price = static_cast<int>(price_float);

    log_info(
        "Drink price was {} (base_price({}) * speakeasy({}) * "
        "cost_mult({}) => {})",
        price, base_price, speakeasy_multiplier, cost_multiplier, price_float);

    int tip = (int) fmax(0, ceil(price * 0.8f * order_info.patience_pct));

    return {price, tip};
}
