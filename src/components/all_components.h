#pragma once

#include <cstdint>
#include <tuple>

// This header is intended to be a single include point for:
// - all component type definitions (useful for serialization/registration)
// - stable snapshot component IDs / iteration helpers
//
// If you add a new component that should be part of "full snapshots", append it
// exactly once in `snapshot_blob::SnapshotComponentTypes`.

// ---- Component type includes (the canonical "all components" list) ----

#include "adds_ingredient.h"
#include "ai_clean_vomit.h"
#include "ai_close_tab.h"
#include "ai_drinking.h"
#include "ai_play_jukebox.h"
#include "ai_use_bathroom.h"
#include "ai_wait_in_queue.h"
#include "ai_wandering.h"
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
#include "can_perform_job.h"
#include "collects_customer_feedback.h"
#include "collects_user_input.h"
#include "conveys_held_item.h"
#include "custom_item_position.h"
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
#include "is_bank.h"
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

// ---- Snapshot kind IDs ----

namespace snapshot_blob {

// Snapshot component types (order matters; keep stable).
//
// IMPORTANT:
// - Never reorder.
// - Only append.
// - This list must stay in lockstep with `ComponentKind` below.
using SnapshotComponentTypes = std::tuple<
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

// Stable component discriminator for snapshots.
// IMPORTANT:
// - Never renumber.
// - Only append.
// - Keep `Transform = 1` (0 reserved for "invalid").
enum class ComponentKind : std::uint16_t {
    Invalid = 0,
    Transform = 1,
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
    AICleanVomit,
};

}  // namespace snapshot_blob

