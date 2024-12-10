
#pragma once

#include "../external_include.h"
//

#include "../components/adds_ingredient.h"
#include "../components/ai_clean_vomit.h"
#include "../components/ai_close_tab.h"
#include "../components/ai_drinking.h"
#include "../components/ai_play_jukebox.h"
#include "../components/ai_use_bathroom.h"
#include "../components/ai_wait_in_queue.h"
#include "../components/ai_wandering.h"
#include "../components/base_component.h"
#include "../components/can_be_ghost_player.h"
#include "../components/can_be_held.h"
#include "../components/can_be_highlighted.h"
#include "../components/can_be_pushed.h"
#include "../components/can_be_taken_from.h"
#include "../components/can_change_settings_interactively.h"
#include "../components/can_grab_from_other_furniture.h"
#include "../components/can_highlight_others.h"
#include "../components/can_hold_furniture.h"
#include "../components/can_hold_handtruck.h"
#include "../components/can_hold_item.h"
#include "../components/can_order_drink.h"
#include "../components/can_pathfind.h"
#include "../components/can_perform_job.h"
#include "../components/collects_customer_feedback.h"
#include "../components/collects_user_input.h"
#include "../components/conveys_held_item.h"
#include "../components/custom_item_position.h"
#include "../components/has_base_speed.h"
#include "../components/has_client_id.h"
#include "../components/has_day_night_timer.h"
#include "../components/has_dynamic_model_name.h"
#include "../components/has_fishing_game.h"
#include "../components/has_last_interacted_customer.h"
#include "../components/has_name.h"
#include "../components/has_patience.h"
#include "../components/has_progression.h"
#include "../components/has_rope_to_item.h"
#include "../components/has_speech_bubble.h"
#include "../components/has_subtype.h"
#include "../components/has_waiting_queue.h"
#include "../components/has_work.h"
#include "../components/indexer.h"
#include "../components/is_bank.h"
#include "../components/is_drink.h"
#include "../components/is_free_in_store.h"
#include "../components/is_item.h"
#include "../components/is_item_container.h"
#include "../components/is_nux_manager.h"
#include "../components/is_pnumatic_pipe.h"
#include "../components/is_progression_manager.h"
#include "../components/is_rotatable.h"
#include "../components/is_round_settings_manager.h"
#include "../components/is_snappable.h"
#include "../components/is_solid.h"
#include "../components/is_spawner.h"
#include "../components/is_squirter.h"
#include "../components/is_store_spawned.h"
#include "../components/is_toilet.h"
#include "../components/is_trigger_area.h"
#include "../components/model_renderer.h"
#include "../components/responds_to_day_night.h"
#include "../components/responds_to_user_input.h"
#include "../components/simple_colored_box_renderer.h"
#include "../components/transform.h"
#include "../components/type.h"
#include "../components/uses_character_model.h"
#include "bitsery/details/serialization_common.h"

//

namespace bitsery {
template<>
struct SelectSerializeFnc<afterhours::BaseComponent> : UseNonMemberFnc {};
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
struct SelectSerializeFnc<Type> : UseMemberFnc {};

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

namespace ext {

template<>
struct PolymorphicBaseClass<BaseComponent>
    : PolymorphicDerivedClasses<
          // BEGIN
          Transform, HasName, CanHoldItem, SimpleColoredBoxRenderer,
          CanBeHighlighted, CanHighlightOthers, CanHoldFurniture,
          CanBeGhostPlayer, CanPerformJob, ModelRenderer, CanBePushed,
          CustomHeldItemPosition, HasWork, HasBaseSpeed, IsSolid, CanBeHeld,
          HasPatience, HasProgression, IsRotatable, CanGrabFromOtherFurniture,
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
          CollectsCustomerFeedback, IsSquirter, Type
          // END
          > {};
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
    bitsery::ext::PolymorphicClassesList<BaseComponent, AIComponent, Job>;
