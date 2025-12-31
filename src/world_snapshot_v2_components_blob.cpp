#include "world_snapshot_v2_components_blob.h"

#include "engine/log.h"

namespace snapshot_v2 {
namespace {

// Keep in sync with src/entity_component_serialization.h
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

using Buffer = std::vector<std::uint8_t>;
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;
using Serializer = bitsery::Serializer<OutputAdapter>;
using Deserializer = bitsery::Deserializer<InputAdapter>;

template<typename S>
void serialize_components_only(S& s, Entity& entity) {
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
void deserialize_components_only_impl(
    bitsery::Deserializer<TAdapter, TContext>& s, Entity& entity) {
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

}  // namespace

std::vector<std::uint8_t> encode_components_blob(const Entity& e) {
    Buffer buf;
    Serializer ser{buf};
    // bitsery's API takes non-const references even for writing.
    Entity& nc = const_cast<Entity&>(e);  // NOLINT
    serialize_components_only(ser, nc);
    ser.adapter().flush();

    if (buf.size() > kMaxEntityComponentsBlobBytes) {
        log_warn("snapshot_v2: components blob is very large: {} bytes", buf.size());
    }

    return buf;
}

void decode_components_blob(Entity& e, const std::vector<std::uint8_t>& blob) {
    Deserializer des{blob.begin(), blob.size()};
    deserialize_components_only_impl(des, e);
    if (des.adapter().error() != bitsery::ReaderError::NoError) {
        log_error("snapshot_v2: decode_components_blob reader_error={}",
                  (int)des.adapter().error());
    }
}

}  // namespace snapshot_v2

