
#pragma once

#include "../external_include.h"
//

#include "../components/all_components.h"
#include "bitsery/details/serialization_common.h"

//

namespace bitsery {
template<>
struct SelectSerializeFnc<afterhours::BaseComponent> : UseNonMemberFnc {};

template<>
struct SelectSerializeFnc<BaseComponent> : UseMemberFnc {};

// Provide a no-op free serialize for the vendor base to satisfy bitsery's
// static checks. Real serialization occurs on derived component types.
template<typename S>
inline void serialize(S&, afterhours::BaseComponent&) {}

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

//

template<>
struct SelectSerializeFnc<AICleanVomit> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<AIUseBathroom> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<AIDrinking> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<AIWaitInQueue> : UseMemberFnc {};
//
template<>
struct SelectSerializeFnc<CanBeHeld_HT> : UseMemberFnc {};
template<>
struct SelectSerializeFnc<BypassAutomationState> : UseMemberFnc {};

namespace ext {

template<>
struct PolymorphicBaseClass<BaseComponent>
    : PolymorphicDerivedClasses<
          // BEGIN
          Transform, HasName, CanHoldItem, SimpleColoredBoxRenderer,
          CanBeHighlighted, CanHighlightOthers, CanHoldFurniture,
          CanBeGhostPlayer, CanPerformJob, ModelRenderer, CanBePushed,
          CustomHeldItemPosition, HasWork, HasBaseSpeed, IsSolid, HasPatience,
          HasProgression, IsRotatable, CanGrabFromOtherFurniture,
          ConveysHeldItem, HasWaitingQueue, CanBeTakenFrom, IsItemContainer,
          UsesCharacterModel, HasDynamicModelName, IsTriggerArea,
          HasSpeechBubble, Indexer, IsSpawner, HasRopeToItem, HasSubtype,
          IsItem, IsDrink, AddsIngredient, CanOrderDrink, IsPnumaticPipe,
          IsProgressionManager, IsFloorMarker, IsBank, IsFreeInStore, IsToilet,
          CanPathfind, IsRoundSettingsManager, AIComponent, HasFishingGame,
          IsStoreSpawned, AICloseTab, AIPlayJukebox, HasLastInteractedCustomer,
          CanChangeSettingsInteractively, IsNuxManager, IsNux, AIWandering,
          CollectsUserInput, IsSnappable, HasClientID, RespondsToUserInput,
          CanHoldHandTruck, RespondsToDayNight, HasDayNightTimer,
          CollectsCustomerFeedback, IsSquirter, CanBeHeld, BypassAutomationState
          // END
          > {};

// Register the vendor base so unique_ptr<afterhours::BaseComponent> works
template<>
struct PolymorphicBaseClass<afterhours::BaseComponent>
    : PolymorphicDerivedClasses<
          // Include BaseComponent itself as an intermediate base
          BaseComponent,
          // Mirror the same concrete components as our BaseComponent
          Transform, HasName, CanHoldItem, SimpleColoredBoxRenderer,
          CanBeHighlighted, CanHighlightOthers, CanHoldFurniture,
          CanBeGhostPlayer, CanPerformJob, ModelRenderer, CanBePushed,
          CustomHeldItemPosition, HasWork, HasBaseSpeed, IsSolid, HasPatience,
          HasProgression, IsRotatable, CanGrabFromOtherFurniture,
          ConveysHeldItem, HasWaitingQueue, CanBeTakenFrom, IsItemContainer,
          UsesCharacterModel, HasDynamicModelName, IsTriggerArea,
          HasSpeechBubble, Indexer, IsSpawner, HasRopeToItem, HasSubtype,
          IsItem, IsDrink, AddsIngredient, CanOrderDrink, IsPnumaticPipe,
          IsProgressionManager, IsFloorMarker, IsBank, IsFreeInStore, IsToilet,
          CanPathfind, IsRoundSettingsManager, AIComponent, HasFishingGame,
          IsStoreSpawned, AICloseTab, AIPlayJukebox, HasLastInteractedCustomer,
          CanChangeSettingsInteractively, IsNuxManager, IsNux, AIWandering,
          CollectsUserInput, IsSnappable, HasClientID, RespondsToUserInput,
          CanHoldHandTruck, RespondsToDayNight, HasDayNightTimer,
          CollectsCustomerFeedback, IsSquirter, CanBeHeld,
          BypassAutomationState> {};
// If you add anything here ^^ then you should add that component to
// register_all_components in entity.h

template<>
struct PolymorphicBaseClass<AIComponent>
    : PolymorphicDerivedClasses<
          // BEGIN
          AICleanVomit, AIUseBathroom, AIDrinking, AIWaitInQueue, AIPlayJukebox,
          AICloseTab, AIWandering
          // END
          > {};

template<>
struct PolymorphicBaseClass<CanBeHeld> : PolymorphicDerivedClasses<
                                             // BEGIN
                                             CanBeHeld_HT
                                             // END
                                             > {};

}  // namespace ext
}  // namespace bitsery

using MyPolymorphicClasses =
    bitsery::ext::PolymorphicClassesList<BaseComponent,
                                         afterhours::BaseComponent, AIComponent,
                                         Job, afterhours::Entity>;
