// Snapshot serialization Bitsery wiring.
//
// This is intentionally *not* "polymorphic" wiring:
// - Save/network formats are pointer-free and do not use Bitsery pointer graphs.
// - Snapshot blob serialization writes concrete component types directly.
//
// This header exists to:
// - include the canonical component list (`all_components.h`)
// - provide Bitsery SelectSerializeFnc specializations for those component types
//
// Keep this header lightweight and scoped to snapshot serialization needs.
#pragma once

#include "../components/all_components.h"
#include "bitsery/details/serialization_common.h"

namespace bitsery {

// Vendor base uses non-member serialize hooks.
template<>
struct SelectSerializeFnc<afterhours::BaseComponent> : UseNonMemberFnc {};

// Our component base uses member serialize hooks.
template<>
struct SelectSerializeFnc<BaseComponent> : UseMemberFnc {};

// Concrete components use member serialize hooks.
template<>
struct SelectSerializeFnc<Transform> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<HasName> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<CanHoldItem> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<SimpleColoredBoxRenderer> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<CanBeHighlighted> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<CanHighlightOthers> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<CanHoldFurniture> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<CanBeGhostPlayer> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<CanPerformJob> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<ModelRenderer> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<CanBePushed> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<CustomHeldItemPosition> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<HasWork> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<HasBaseSpeed> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<IsSolid> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<CanBeHeld> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<HasPatience> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<HasProgression> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<IsRotatable> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<CanGrabFromOtherFurniture> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<ConveysHeldItem> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<HasWaitingQueue> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<CanBeTakenFrom> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<IsItemContainer> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<UsesCharacterModel> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<HasDynamicModelName> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<IsTriggerArea> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<HasSpeechBubble> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<Indexer> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<IsSpawner> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<HasRopeToItem> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<HasSubtype> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<IsItem> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<IsDrink> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<AddsIngredient> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<CanOrderDrink> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<IsPnumaticPipe> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<IsProgressionManager> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<IsFloorMarker> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<IsBank> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<IsFreeInStore> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<IsToilet> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<CanPathfind> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<IsRoundSettingsManager> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<AIComponent> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<HasFishingGame> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<IsStoreSpawned> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<AICloseTab> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<AIPlayJukebox> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<HasLastInteractedCustomer> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<CanChangeSettingsInteractively> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<IsNuxManager> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<IsNux> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<AIWandering> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<CollectsUserInput> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<IsSnappable> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<HasClientID> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<RespondsToUserInput> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<CanHoldHandTruck> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<RespondsToDayNight> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<HasDayNightTimer> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<CollectsCustomerFeedback> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<IsSquirter> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<AICleanVomit> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<AIUseBathroom> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<AIDrinking> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<AIWaitInQueue> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<CanBeHeld_HT> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<BypassAutomationState> : UseMemberFnc {};

}  // namespace bitsery

