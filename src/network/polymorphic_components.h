
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
#include "../components/collects_user_input.h"
#include "../components/conveys_held_item.h"
#include "../components/custom_item_position.h"
#include "../components/has_base_speed.h"
#include "../components/has_client_id.h"
#include "../components/has_dynamic_model_name.h"
#include "../components/has_fishing_game.h"
#include "../components/has_last_interacted_customer.h"
#include "../components/has_name.h"
#include "../components/has_patience.h"
#include "../components/has_progression.h"
#include "../components/has_rope_to_item.h"
#include "../components/has_speech_bubble.h"
#include "../components/has_subtype.h"
#include "../components/has_timer.h"
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
#include "../components/is_store_spawned.h"
#include "../components/is_toilet.h"
#include "../components/is_trigger_area.h"
#include "../components/model_renderer.h"
#include "../components/responds_to_user_input.h"
#include "../components/simple_colored_box_renderer.h"
#include "../components/transform.h"
#include "../components/uses_character_model.h"

//

namespace bitsery {

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
          HasSpeechBubble, Indexer, IsSpawner, HasTimer, HasRopeToItem,
          HasSubtype, IsItem, IsDrink, AddsIngredient, CanOrderDrink,
          IsPnumaticPipe, IsProgressionManager, IsFloorMarker, IsBank,
          IsFreeInStore, IsToilet, CanPathfind, IsRoundSettingsManager,
          AIComponent, HasFishingGame, IsStoreSpawned, AICloseTab,
          AIPlayJukebox, HasLastInteractedCustomer,
          CanChangeSettingsInteractively, IsNuxManager, IsNux, AIWandering,
          CollectsUserInput, IsSnappable, HasClientID, RespondsToUserInput,
          CanHoldHandTruck
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
