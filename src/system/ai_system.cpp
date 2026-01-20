#include "ai_system.h"

#include <functional>
#include <optional>

#include "../building_locations.h"
#include "../components/ai_wait_in_queue_state.h"
#include "../components/can_hold_item.h"
#include "../components/can_order_drink.h"
#include "../components/can_pathfind.h"
#include "../components/has_ai_cooldown.h"
#include "../components/has_ai_target_location.h"
#include "../components/has_base_speed.h"
#include "../components/has_waiting_queue.h"
#include "../components/is_round_settings_manager.h"
#include "../components/transform.h"
#include "../engine/assert.h"
#include "../engine/log.h"
#include "../engine/runtime_globals.h"
#include "../entity.h"
#include "../entity_helper.h"
#include "../recipe_library.h"
#include "ai_entity_helpers.h"

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

// Rate-limit AI work in states that don't need per-frame processing.
[[nodiscard]] inline bool ai_tick_with_cooldown(Entity& entity, float dt,
                                                float reset_to_seconds) {
    HasAICooldown& cd = entity.addComponentIfMissing<HasAICooldown>();
    cd.cooldown.reset_to = reset_to_seconds;
    cd.cooldown.tick(dt);
    if (!cd.cooldown.ready()) return false;
    cd.cooldown.reset();
    return true;
}

[[nodiscard]] inline std::optional<vec2> pick_random_walkable_near(
    const Entity& e, int attempts = 50) {
    const vec2 base = e.get<Transform>().as2();
    for (int i = 0; i < attempts; ++i) {
        vec2 off = RandomEngine::get().get_vec(-10.f, 10.f);
        vec2 p = base + off;
        if (EntityHelper::isWalkable(p)) return p;
    }
    return std::nullopt;
}

[[nodiscard]] inline std::optional<vec2> pick_random_walkable_in_building(
    const Entity& e, const Building& building, int attempts = 50) {
    const vec2 base = e.get<Transform>().as2();
    for (int i = 0; i < attempts; ++i) {
        vec2 off = RandomEngine::get().get_vec(-10.f, 10.f);
        vec2 p = base + off;
        if (EntityHelper::isWalkable(p) && building.is_inside(p)) return p;
    }
    return std::nullopt;
}

// ---- Queue/line helpers (system logic; queue state is data-only) ----
inline void line_reset(Entity& entity, AIWaitInQueueState& s) {
    s.has_set_position_before = false;
    s.previous_line_index = -1;
    s.queue_index = -1;
    if (entity.has<HasAITargetLocation>()) {
        entity.get<HasAITargetLocation>().pos.reset();
    }
}

inline void line_add_to_queue(Entity& entity, AIWaitInQueueState& s,
                              Entity& reg) {
    VALIDATE(reg.has<HasWaitingQueue>(),
             "Trying to add_to_queue for entity which doesn't have a waiting "
             "queue");
    HasWaitingQueue& hwq = reg.get<HasWaitingQueue>();
    int next_position = hwq.add_customer(entity).get_next_pos();
    HasAITargetLocation& tl =
        entity.addComponentIfMissing<HasAITargetLocation>();
    tl.pos = reg.get<Transform>().tile_infront((next_position + 1));
    s.has_set_position_before = true;
}

[[nodiscard]] inline int line_position_in_line(AIWaitInQueueState& s,
                                               Entity& reg,
                                               const Entity& entity) {
    VALIDATE(reg.has<HasWaitingQueue>(),
             "Trying to position_in_line for entity which doesn't have a "
             "waiting queue");
    const HasWaitingQueue& hwq = reg.get<HasWaitingQueue>();
    s.previous_line_index = hwq.get_customer_position(entity.id);
    s.queue_index = s.previous_line_index;
    return s.previous_line_index;
}

[[nodiscard]] inline bool line_can_move_up(const Entity& reg,
                                           const Entity& customer) {
    VALIDATE(reg.has<HasWaitingQueue>(),
             "Trying to can_move_up for entity which doesn't have a waiting "
             "queue");
    return reg.get<HasWaitingQueue>().matching_id(customer.id, 0);
}

// Returns true when the entity reaches the front of the line.
[[nodiscard]] inline bool line_try_to_move_closer(
    AIWaitInQueueState& s, Entity& reg, Entity& entity, float distance,
    const std::function<void()>& onReachedFront = nullptr) {
    if (!s.has_set_position_before) {
        log_error("AI line state: add_to_queue must be called first");
    }

    HasAITargetLocation& tl =
        entity.addComponentIfMissing<HasAITargetLocation>();
    if (!tl.pos.has_value()) {
        tl.pos = reg.get<Transform>().tile_directly_infront();
    }
    bool travel_result =
        entity.get<CanPathfind>().travel_toward(tl.pos.value(), distance);

    int spot_in_line = line_position_in_line(s, reg, entity);
    if (spot_in_line != 0) {
        if (!line_can_move_up(reg, entity)) {
            return false;
        }
        // Walk up one spot.
        tl.pos = reg.get<Transform>().tile_infront(spot_in_line);
        return false;
    }

    // At position 0 (front of line), but must verify we've actually reached the
    // target Only return true if we're actually at the target location
    if (!travel_result) {
        // Still moving toward target, update target to directly in front and
        // continue
        tl.pos = reg.get<Transform>().tile_directly_infront();
        return false;
    }

    tl.pos = reg.get<Transform>().tile_directly_infront();
    if (onReachedFront) onReachedFront();
    return true;
}

inline void line_leave(AIWaitInQueueState& s, Entity& reg,
                       const Entity& entity) {
    VALIDATE(reg.has<HasWaitingQueue>(),
             "Trying to leave_line for entity which doesn't have a waiting "
             "queue");
    int pos = line_position_in_line(s, reg, entity);
    if (pos == -1) return;
    reg.get<HasWaitingQueue>().erase(pos);
}

}  // namespace system_manager::ai
