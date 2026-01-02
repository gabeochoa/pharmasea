#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <tuple>
#include <utility>

#include "../../vendor/afterhours/src/type_name.h"

// This file defines the snapshot schema: the stable ordered list of component
// types used for save/replay/network snapshots.
//
// IMPORTANT:
// - Never reorder.
// - Only append.
// - Deleting or reordering entries will break save files / replays / network
//   snapshot compatibility because the on-wire snapshot format assumes a stable
//   component ID assignment (Afterhours static ids) and a stable serialization
//   order based on this list.

// Forward declarations for all snapshot-relevant component types.
struct Transform;
struct HasName;
struct CanHoldItem;
struct SimpleColoredBoxRenderer;
struct CanBeHighlighted;
struct CanHighlightOthers;
struct CanHoldFurniture;
struct CanBeGhostPlayer;
struct CanPerformJob;
struct ModelRenderer;
struct CanBePushed;
struct CustomHeldItemPosition;
struct HasWork;
struct HasBaseSpeed;
struct IsSolid;
struct HasPatience;
struct HasProgression;
struct IsRotatable;
struct CanGrabFromOtherFurniture;
struct ConveysHeldItem;
struct HasWaitingQueue;
struct CanBeTakenFrom;
struct IsItemContainer;
struct UsesCharacterModel;
struct HasDynamicModelName;
struct IsTriggerArea;
struct HasSpeechBubble;
struct Indexer;
struct IsSpawner;
struct HasRopeToItem;
struct HasSubtype;
struct IsItem;
struct IsDrink;
struct AddsIngredient;
struct CanOrderDrink;
struct IsPnumaticPipe;
struct IsProgressionManager;
struct IsFloorMarker;
struct IsBank;
struct IsFreeInStore;
struct IsToilet;
struct CanPathfind;
struct IsRoundSettingsManager;
struct HasFishingGame;
struct IsStoreSpawned;
struct HasLastInteractedCustomer;
struct CanChangeSettingsInteractively;
struct IsNuxManager;
struct IsNux;
struct CollectsUserInput;
struct IsSnappable;
struct HasClientID;
struct RespondsToUserInput;
struct CanHoldHandTruck;
struct RespondsToDayNight;
struct HasDayNightTimer;
struct CollectsCustomerFeedback;
struct IsSquirter;
struct CanBeHeld_HT;
struct CanBeHeld;
struct BypassAutomationState;
struct AICloseTab;
struct AIPlayJukebox;
struct AIWaitInQueue;
struct AIDrinking;
struct AIUseBathroom;
struct AIWandering;
struct AICleanVomit;

namespace snapshot_blob {

// The canonical ordered component list for snapshots.
using ComponentTypes = std::tuple<
    Transform,
    HasName,
    CanHoldItem,
    SimpleColoredBoxRenderer,
    CanBeHighlighted,
    CanHighlightOthers,
    CanHoldFurniture,
    CanBeGhostPlayer,
    CanPerformJob,
    ModelRenderer,
    CanBePushed,
    CustomHeldItemPosition,
    HasWork,
    HasBaseSpeed,
    IsSolid,
    HasPatience,
    HasProgression,
    IsRotatable,
    CanGrabFromOtherFurniture,
    ConveysHeldItem,
    HasWaitingQueue,
    CanBeTakenFrom,
    IsItemContainer,
    UsesCharacterModel,
    HasDynamicModelName,
    IsTriggerArea,
    HasSpeechBubble,
    Indexer,
    IsSpawner,
    HasRopeToItem,
    HasSubtype,
    IsItem,
    IsDrink,
    AddsIngredient,
    CanOrderDrink,
    IsPnumaticPipe,
    IsProgressionManager,
    IsFloorMarker,
    IsBank,
    IsFreeInStore,
    IsToilet,
    CanPathfind,
    IsRoundSettingsManager,
    HasFishingGame,
    IsStoreSpawned,
    HasLastInteractedCustomer,
    CanChangeSettingsInteractively,
    IsNuxManager,
    IsNux,
    CollectsUserInput,
    IsSnappable,
    HasClientID,
    RespondsToUserInput,
    CanHoldHandTruck,
    RespondsToDayNight,
    HasDayNightTimer,
    CollectsCustomerFeedback,
    IsSquirter,
    CanBeHeld_HT,
    CanBeHeld,
    BypassAutomationState,
    AICloseTab,
    AIPlayJukebox,
    AIWaitInQueue,
    AIDrinking,
    AIUseBathroom,
    AIWandering,
    AICleanVomit>;

// Compile-time checksum for `ComponentTypes` order.
// This is intended for build-time "acknowledgement" gates.
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

