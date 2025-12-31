#pragma once

// This header provides **value** (ComponentStore-backed) serialization for
// `afterhours::Entity` when using bitsery Serializer/Deserializer. It is meant
// to be included only in translation units that actually serialize entities
// (network/save), to avoid heavy include cycles in `entity.h`.

#include "entity.h"

// Provides concrete component type declarations + bitsery serialize support.
#include "network/polymorphic_components.h"

namespace bitsery {

// NOTE: We encode presence bits + component payloads in a fixed order so the
// componentSet can be reconstructed on load without storing per-entity pointers.
//
// This is a transitional step while moving to pointer-free snapshot payloads.

#define PHARMASEA_FOR_EACH_COMPONENT_TYPE(X)                                    \
    X(Transform)                                                               \
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
    X(CanBeHeld)                                                               \
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
    X(AIComponent)                                                             \
    X(HasFishingGame)                                                          \
    X(IsStoreSpawned)                                                          \
    X(AICloseTab)                                                              \
    X(AIPlayJukebox)                                                           \
    X(HasLastInteractedCustomer)                                               \
    X(CanChangeSettingsInteractively)                                          \
    X(IsNuxManager)                                                            \
    X(IsNux)                                                                   \
    X(AIWandering)                                                             \
    X(CollectsUserInput)                                                       \
    X(IsSnappable)                                                             \
    X(HasClientID)                                                             \
    X(RespondsToUserInput)                                                     \
    X(CanHoldHandTruck)                                                        \
    X(RespondsToDayNight)                                                      \
    X(HasDayNightTimer)                                                        \
    X(CollectsCustomerFeedback)                                                \
    X(IsSquirter)                                                              \
    X(AICleanVomit)                                                            \
    X(AIUseBathroom)                                                           \
    X(AIDrinking)                                                              \
    X(AIWaitInQueue)                                                           \
    X(CanBeHeld_HT)                                                            \
    X(BypassAutomationState)

template<typename TAdapter, typename TContext>
inline void serialize(bitsery::Serializer<TAdapter, TContext>& s,
                      afterhours::Entity& entity) {
    s.value4b(entity.id);
    s.value4b(entity.entity_type);
    s.ext(entity.tags, bitsery::ext::StdBitset{});
    s.value1b(entity.cleanup);

#define PHARMASEA_WRITE_COMPONENT(T)                                            \
    do {                                                                        \
        bool has = entity.has<T>();                                             \
        s.value1b(has);                                                         \
        if (has) {                                                              \
            s.object(entity.get<T>());                                          \
        }                                                                       \
    } while (0);
    PHARMASEA_FOR_EACH_COMPONENT_TYPE(PHARMASEA_WRITE_COMPONENT)
#undef PHARMASEA_WRITE_COMPONENT
}

template<typename TAdapter, typename TContext>
inline void serialize(bitsery::Deserializer<TAdapter, TContext>& s,
                      afterhours::Entity& entity) {
    s.value4b(entity.id);
    s.value4b(entity.entity_type);
    s.ext(entity.tags, bitsery::ext::StdBitset{});
    s.value1b(entity.cleanup);

    // Rebuild componentSet by adding components as we read them.
    entity.componentSet.reset();

#define PHARMASEA_READ_COMPONENT(T)                                             \
    do {                                                                        \
        bool has = false;                                                       \
        s.value1b(has);                                                         \
        if (has) {                                                              \
            entity.addComponent<T>();                                           \
            s.object(entity.get<T>());                                          \
        }                                                                       \
    } while (0);
    PHARMASEA_FOR_EACH_COMPONENT_TYPE(PHARMASEA_READ_COMPONENT)
#undef PHARMASEA_READ_COMPONENT
}

#undef PHARMASEA_FOR_EACH_COMPONENT_TYPE

}  // namespace bitsery

