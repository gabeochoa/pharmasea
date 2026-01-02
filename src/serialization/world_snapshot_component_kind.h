#pragma once

#include <cstdint>

// Single source of truth for:
// - which components are included in "full snapshot" surfaces
// - stable on-wire component kind IDs (must never be renumbered)
//
// Anything that wants to iterate over snapshot components should use
// `PHARMASEA_SNAPSHOT_COMPONENT_LIST(X)`.

// Full list (order matters; keep stable).
// NOTE: this macro expands to statements, so it can be used to generate:
// - enum entries
// - registration calls
// - serializer dispatch tables
#define PHARMASEA_SNAPSHOT_COMPONENT_LIST_REST(X)                               \
    X(HasName)                                                                 \
    X(CanHoldItem)                                                             \
    X(SimpleColoredBoxRenderer)                                                \
    X(CanBeHighlighted)                                                        \
    X(CanHighlightOthers)                                                      \
    X(CanHoldFurniture)                                                        \
    X(CanBeGhostPlayer)                                                        \
    X(CanPerformJob)                                                           \
    X(ModelRenderer)                                                           \
    X(CanBePushed)                                                             \
    X(CustomHeldItemPosition)                                                  \
    X(HasWork)                                                                 \
    X(HasBaseSpeed)                                                            \
    X(IsSolid)                                                                 \
    X(HasPatience)                                                             \
    X(HasProgression)                                                          \
    X(IsRotatable)                                                             \
    X(CanGrabFromOtherFurniture)                                               \
    X(ConveysHeldItem)                                                         \
    X(HasWaitingQueue)                                                         \
    X(CanBeTakenFrom)                                                          \
    X(IsItemContainer)                                                         \
    X(UsesCharacterModel)                                                      \
    X(HasDynamicModelName)                                                     \
    X(IsTriggerArea)                                                           \
    X(HasSpeechBubble)                                                         \
    X(Indexer)                                                                 \
    X(IsSpawner)                                                               \
    X(HasRopeToItem)                                                           \
    X(HasSubtype)                                                              \
    X(IsItem)                                                                  \
    X(IsDrink)                                                                 \
    X(AddsIngredient)                                                          \
    X(CanOrderDrink)                                                           \
    X(IsPnumaticPipe)                                                          \
    X(IsProgressionManager)                                                    \
    X(IsFloorMarker)                                                           \
    X(IsBank)                                                                  \
    X(IsFreeInStore)                                                           \
    X(IsToilet)                                                                \
    X(CanPathfind)                                                             \
    X(IsRoundSettingsManager)                                                  \
    X(HasFishingGame)                                                          \
    X(IsStoreSpawned)                                                          \
    X(HasLastInteractedCustomer)                                               \
    X(CanChangeSettingsInteractively)                                          \
    X(IsNuxManager)                                                            \
    X(IsNux)                                                                   \
    X(CollectsUserInput)                                                       \
    X(IsSnappable)                                                             \
    X(HasClientID)                                                             \
    X(RespondsToUserInput)                                                     \
    X(CanHoldHandTruck)                                                        \
    X(RespondsToDayNight)                                                      \
    X(HasDayNightTimer)                                                        \
    X(CollectsCustomerFeedback)                                                \
    X(IsSquirter)                                                              \
    X(CanBeHeld_HT)                                                            \
    X(CanBeHeld)                                                               \
    X(BypassAutomationState)                                                   \
    X(AICloseTab)                                                              \
    X(AIPlayJukebox)                                                           \
    X(AIWaitInQueue)                                                           \
    X(AIDrinking)                                                              \
    X(AIUseBathroom)                                                           \
    X(AIWandering)                                                             \
    X(AICleanVomit)

#define PHARMASEA_SNAPSHOT_COMPONENT_LIST(X)                                    \
    X(Transform)                                                               \
    PHARMASEA_SNAPSHOT_COMPONENT_LIST_REST(X)

namespace snapshot_blob {

// Stable component discriminator for snapshots.
// IMPORTANT:
// - Never renumber.
// - Only append.
// - Keep `Transform = 1` (0 reserved for "invalid").
enum class ComponentKind : std::uint16_t {
    Invalid = 0,
    Transform = 1,
#define PHARMASEA_KIND_ENUM_ENTRY(T) T,
    PHARMASEA_SNAPSHOT_COMPONENT_LIST_REST(PHARMASEA_KIND_ENUM_ENTRY)
#undef PHARMASEA_KIND_ENUM_ENTRY
};

}  // namespace snapshot_blob

