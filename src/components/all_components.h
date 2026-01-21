#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <tuple>
#include <utility>

#include "../../vendor/afterhours/src/type_name.h"

// This header is intended to be a single include point for:
// - all component type definitions (useful for serialization/registration)
// - stable snapshot schema helpers (component snapshot order + checksum)
//
// If you add a new component that should be part of "full snapshots", append it
// exactly once in `snapshot_blob::ComponentTypes` below.

// ---- Component type includes (the canonical "all components" list) ----

#include "adds_ingredient.h"
#include "base_component.h"
#include "bypass_automation_state.h"
#include "can_be_ghost_player.h"
#include "can_be_held.h"
#include "can_be_highlighted.h"
#include "can_be_pushed.h"
#include "can_be_taken_from.h"
#include "can_change_settings_interactively.h"
#include "can_grab_from_other_furniture.h"
#include "can_highlight_others.h"
#include "can_hold_furniture.h"
#include "can_hold_handtruck.h"
#include "can_hold_item.h"
#include "can_order_drink.h"
#include "can_pathfind.h"
#include "collects_customer_feedback.h"
#include "collects_user_input.h"
#include "player_input_queue.h"
#include "conveys_held_item.h"
#include "custom_item_position.h"
#include "has_ai_bathroom_state.h"
#include "has_ai_cooldown.h"
#include "has_ai_drink_state.h"
#include "has_ai_jukebox_state.h"
#include "has_ai_pay_state.h"
#include "has_ai_queue_state.h"
#include "has_ai_target_entity.h"
#include "has_ai_target_location.h"
#include "has_ai_wander_state.h"
#include "has_base_speed.h"
#include "has_client_id.h"
#include "has_day_night_timer.h"
#include "has_dynamic_model_name.h"
#include "has_fishing_game.h"
#include "has_last_interacted_customer.h"
#include "has_name.h"
#include "has_patience.h"
#include "has_progression.h"
#include "has_rope_to_item.h"
#include "has_speech_bubble.h"
#include "has_subtype.h"
#include "has_waiting_queue.h"
#include "has_work.h"
#include "indexer.h"
#include "is_ai_controlled.h"
#include "is_bank.h"
#include "is_customer.h"
#include "is_drink.h"
#include "is_free_in_store.h"
#include "is_item.h"
#include "is_item_container.h"
#include "is_nux_manager.h"
#include "is_pnumatic_pipe.h"
#include "is_progression_manager.h"
#include "is_rotatable.h"
#include "is_round_settings_manager.h"
#include "is_snappable.h"
#include "is_solid.h"
#include "is_spawner.h"
#include "is_squirter.h"
#include "is_store_spawned.h"
#include "is_toilet.h"
#include "is_trigger_area.h"
#include "model_renderer.h"
#include "responds_to_day_night.h"
#include "responds_to_user_input.h"
#include "simple_colored_box_renderer.h"
#include "transform.h"
#include "uses_character_model.h"

// ---- Snapshot schema (ComponentTypes list + checksum) ----

namespace snapshot_blob {

// Snapshot component types (order matters; keep stable).
//
// IMPORTANT:
// - Never reorder.
// - Only append.
// - Deleting or reordering entries will break save files / replays / network
//   snapshot compatibility because the on-wire snapshot format serializes
//   component payloads positionally in this exact order.
using ComponentTypes = std::tuple<
    Transform, HasName, CanHoldItem, SimpleColoredBoxRenderer, CanBeHighlighted,
    CanHighlightOthers, CanHoldFurniture, CanBeGhostPlayer, IsAIControlled,
    ModelRenderer, CanBePushed, CustomHeldItemPosition, HasWork, HasBaseSpeed,
    IsSolid, HasPatience, HasProgression, IsRotatable,
    CanGrabFromOtherFurniture, ConveysHeldItem, HasWaitingQueue, CanBeTakenFrom,
    IsItemContainer, UsesCharacterModel, HasDynamicModelName, IsTriggerArea,
    HasSpeechBubble, Indexer, IsSpawner, HasRopeToItem, HasSubtype, IsItem,
    IsDrink, AddsIngredient, CanOrderDrink, IsPnumaticPipe,
    IsProgressionManager, IsFloorMarker, IsBank, IsFreeInStore, IsToilet,
    CanPathfind, IsRoundSettingsManager, HasFishingGame, IsStoreSpawned,
    HasLastInteractedCustomer, CanChangeSettingsInteractively, IsNuxManager,
    IsNux, CollectsUserInput, IsSnappable, HasClientID, RespondsToUserInput,
    CanHoldHandTruck, RespondsToDayNight, HasDayNightTimer,
    CollectsCustomerFeedback, IsSquirter, CanBeHeld_HT, CanBeHeld,
    BypassAutomationState,
    // ---- Consolidated AI (Part B) ----
    HasAICooldown, HasAITargetEntity, HasAITargetLocation, HasAIQueueState,
    HasAIDrinkState, HasAIBathroomState, HasAIPayState, HasAIJukeboxState,
    HasAIWanderState, IsCustomer>;

namespace detail {
constexpr std::uint64_t kFnv1aOffset = 14695981039346656037ull;
constexpr std::uint64_t kFnv1aPrime = 1099511628211ull;

constexpr std::uint64_t fnv1a_step(std::uint64_t h, std::uint8_t b) {
    return (h ^ b) * kFnv1aPrime;
}

constexpr std::uint64_t fnv1a_append(std::uint64_t h, std::string_view s) {
    for (char c : s) {
        h = fnv1a_step(h, static_cast<std::uint8_t>(c));
    }
    return h;
}

template<typename Tuple, size_t... Is>
constexpr auto tuple_type_names(std::index_sequence<Is...>) {
    return std::array<std::string_view, sizeof...(Is)>{
        ::type_name<std::tuple_element_t<Is, Tuple>>()...,
    };
}
}  // namespace detail

inline constexpr std::uint64_t component_types_checksum() {
    constexpr auto names = detail::tuple_type_names<ComponentTypes>(
        std::make_index_sequence<std::tuple_size_v<ComponentTypes>>{});
    std::uint64_t h = detail::kFnv1aOffset;
    h = detail::fnv1a_append(h, "ComponentTypesChecksum.v1");
    h = detail::fnv1a_step(h, 0);
    for (auto n : names) {
        h = detail::fnv1a_append(h, n);
        h = detail::fnv1a_step(h, 0);
    }
    return h;
}

inline constexpr std::uint64_t kComponentTypesChecksum =
    component_types_checksum();

}  // namespace snapshot_blob
