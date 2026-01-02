#include "world_snapshot_blob.h"

#include "../bitsery_include.h"
#include "../engine/log.h"
#include "../entity_helper.h"
#include "../network/polymorphic_components.h"  // brings in all component types + bitsery config

#include <bitsery/ext/std_bitset.h>

namespace snapshot_blob {

namespace {
using Buffer = std::string;
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;
using TContext = std::tuple<>;
using Serializer = bitsery::Serializer<OutputAdapter, TContext>;
using Deserializer = bitsery::Deserializer<InputAdapter, TContext>;

// Stable component discriminator for snapshots.
// IMPORTANT: never renumber these once shipped; add new values at the end.
enum class ComponentKind : std::uint16_t {
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

struct ComponentBlobDTO {
    ComponentKind kind = ComponentKind::Transform;
    std::string bytes;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        std::uint16_t k = static_cast<std::uint16_t>(kind);
        s.value2b(k);
        if constexpr (requires { s.adapter().error(); }) {
            // Reader: restore kind.
            kind = static_cast<ComponentKind>(k);
        }

        std::uint32_t sz = 0;
        if constexpr (requires { s.adapter().error(); }) {
            s.value4b(sz);
            VALIDATE(sz <= snapshot_blob::kMaxEntitySnapshotBytes,
                     "component blob too large");
            bytes.resize(sz);
        } else {
            sz = static_cast<std::uint32_t>(bytes.size());
            s.value4b(sz);
        }
        for (std::uint32_t i = 0; i < sz; ++i) {
            s.value1b(bytes[i]);
        }
    }
};

struct EntitySnapshotDTO {
    afterhours::EntityID id = -1;
    int entity_type = 0;
    afterhours::TagBitset tags{};
    bool cleanup = false;

    std::vector<ComponentBlobDTO> components;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value4b(id);
        s.value4b(entity_type);
        s.ext(tags, bitsery::ext::StdBitset{});
        s.value1b(cleanup);

        uint8_t num_components = 0;
        if constexpr (requires { s.adapter().error(); }) {
            s.value1b(num_components);
            components.resize(num_components);
        } else {
            num_components = static_cast<uint8_t>(components.size());
            s.value1b(num_components);
        }

        for (uint8_t i = 0; i < num_components; ++i) s.object(components[i]);
    }
};

struct WorldSnapshotDTO {
    // Explicit version to allow future migration if needed.
    uint32_t version = 1;
    std::vector<EntitySnapshotDTO> entities;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value4b(version);

        uint32_t num_entities = 0;
        if constexpr (requires { s.adapter().error(); }) {
            s.value4b(num_entities);
            entities.resize(num_entities);
        } else {
            num_entities = static_cast<uint32_t>(entities.size());
            s.value4b(num_entities);
        }

        for (uint32_t i = 0; i < num_entities; ++i) {
            s.object(entities[i]);
        }
    }
};

void clear_all_components(afterhours::Entity& e) {
    e.componentSet.reset();
    for (auto& ptr : e.componentArray) {
        ptr.reset();
    }
}

template<typename T>
[[nodiscard]] std::string encode_component(T& cmp) {
    Buffer buffer;
    TContext ctx{};
    Serializer ser{ctx, buffer};
    ser.object(cmp);
    ser.adapter().flush();
    return buffer;
}

template<typename T>
[[nodiscard]] bool decode_component_into(T& cmp, const std::string& bytes) {
    TContext ctx{};
    Deserializer des{ctx, bytes.begin(), bytes.size()};
    des.object(cmp);
    return des.adapter().error() == bitsery::ReaderError::NoError;
}

template<typename T>
void push_component_blob(EntitySnapshotDTO& out, ComponentKind kind, T& cmp) {
    out.components.push_back(ComponentBlobDTO{
        .kind = kind,
        .bytes = encode_component(cmp),
    });
}

EntitySnapshotDTO snapshot_entity(const afterhours::Entity& e) {
    EntitySnapshotDTO out;
    out.id = e.id;
    out.entity_type = e.entity_type;
    out.tags = e.tags;
    out.cleanup = e.cleanup;

    out.components.reserve(afterhours::max_num_components);

    // Gather all known component types. (Phase 1: full snapshot.)
    //
    // NOTE: for derived-component families (e.g., AIComponent), we list the
    // concrete derived types explicitly.
    auto& nc = const_cast<afterhours::Entity&>(e);

    // Manual expansion keeps ordering stable and avoids macro tricks that
    // complicate the variant type list.
#define ADD_IF_HAS(T, K) \
    do {                 \
        if (nc.has<T>()) push_component_blob(out, (K), nc.get<T>()); \
    } while (0)

    ADD_IF_HAS(Transform, ComponentKind::Transform);
    ADD_IF_HAS(HasName, ComponentKind::HasName);
    ADD_IF_HAS(CanHoldItem, ComponentKind::CanHoldItem);
    ADD_IF_HAS(SimpleColoredBoxRenderer, ComponentKind::SimpleColoredBoxRenderer);
    ADD_IF_HAS(CanBeHighlighted, ComponentKind::CanBeHighlighted);
    ADD_IF_HAS(CanHighlightOthers, ComponentKind::CanHighlightOthers);
    ADD_IF_HAS(CanHoldFurniture, ComponentKind::CanHoldFurniture);
    ADD_IF_HAS(CanBeGhostPlayer, ComponentKind::CanBeGhostPlayer);
    ADD_IF_HAS(CanPerformJob, ComponentKind::CanPerformJob);
    ADD_IF_HAS(ModelRenderer, ComponentKind::ModelRenderer);
    ADD_IF_HAS(CanBePushed, ComponentKind::CanBePushed);
    ADD_IF_HAS(CustomHeldItemPosition, ComponentKind::CustomHeldItemPosition);
    ADD_IF_HAS(HasWork, ComponentKind::HasWork);
    ADD_IF_HAS(HasBaseSpeed, ComponentKind::HasBaseSpeed);
    ADD_IF_HAS(IsSolid, ComponentKind::IsSolid);
    ADD_IF_HAS(HasPatience, ComponentKind::HasPatience);
    ADD_IF_HAS(HasProgression, ComponentKind::HasProgression);
    ADD_IF_HAS(IsRotatable, ComponentKind::IsRotatable);
    ADD_IF_HAS(CanGrabFromOtherFurniture, ComponentKind::CanGrabFromOtherFurniture);
    ADD_IF_HAS(ConveysHeldItem, ComponentKind::ConveysHeldItem);
    ADD_IF_HAS(HasWaitingQueue, ComponentKind::HasWaitingQueue);
    ADD_IF_HAS(CanBeTakenFrom, ComponentKind::CanBeTakenFrom);
    ADD_IF_HAS(IsItemContainer, ComponentKind::IsItemContainer);
    ADD_IF_HAS(UsesCharacterModel, ComponentKind::UsesCharacterModel);
    ADD_IF_HAS(HasDynamicModelName, ComponentKind::HasDynamicModelName);
    ADD_IF_HAS(IsTriggerArea, ComponentKind::IsTriggerArea);
    ADD_IF_HAS(HasSpeechBubble, ComponentKind::HasSpeechBubble);
    ADD_IF_HAS(Indexer, ComponentKind::Indexer);
    ADD_IF_HAS(IsSpawner, ComponentKind::IsSpawner);
    ADD_IF_HAS(HasRopeToItem, ComponentKind::HasRopeToItem);
    ADD_IF_HAS(HasSubtype, ComponentKind::HasSubtype);
    ADD_IF_HAS(IsItem, ComponentKind::IsItem);
    ADD_IF_HAS(IsDrink, ComponentKind::IsDrink);
    ADD_IF_HAS(AddsIngredient, ComponentKind::AddsIngredient);
    ADD_IF_HAS(CanOrderDrink, ComponentKind::CanOrderDrink);
    ADD_IF_HAS(IsPnumaticPipe, ComponentKind::IsPnumaticPipe);
    ADD_IF_HAS(IsProgressionManager, ComponentKind::IsProgressionManager);
    ADD_IF_HAS(IsFloorMarker, ComponentKind::IsFloorMarker);
    ADD_IF_HAS(IsBank, ComponentKind::IsBank);
    ADD_IF_HAS(IsFreeInStore, ComponentKind::IsFreeInStore);
    ADD_IF_HAS(IsToilet, ComponentKind::IsToilet);
    ADD_IF_HAS(CanPathfind, ComponentKind::CanPathfind);
    ADD_IF_HAS(IsRoundSettingsManager, ComponentKind::IsRoundSettingsManager);
    ADD_IF_HAS(HasFishingGame, ComponentKind::HasFishingGame);
    ADD_IF_HAS(IsStoreSpawned, ComponentKind::IsStoreSpawned);
    ADD_IF_HAS(HasLastInteractedCustomer, ComponentKind::HasLastInteractedCustomer);
    ADD_IF_HAS(CanChangeSettingsInteractively, ComponentKind::CanChangeSettingsInteractively);
    ADD_IF_HAS(IsNuxManager, ComponentKind::IsNuxManager);
    ADD_IF_HAS(IsNux, ComponentKind::IsNux);
    ADD_IF_HAS(CollectsUserInput, ComponentKind::CollectsUserInput);
    ADD_IF_HAS(IsSnappable, ComponentKind::IsSnappable);
    ADD_IF_HAS(HasClientID, ComponentKind::HasClientID);
    ADD_IF_HAS(RespondsToUserInput, ComponentKind::RespondsToUserInput);
    ADD_IF_HAS(CanHoldHandTruck, ComponentKind::CanHoldHandTruck);
    ADD_IF_HAS(RespondsToDayNight, ComponentKind::RespondsToDayNight);
    ADD_IF_HAS(HasDayNightTimer, ComponentKind::HasDayNightTimer);
    ADD_IF_HAS(CollectsCustomerFeedback, ComponentKind::CollectsCustomerFeedback);
    ADD_IF_HAS(IsSquirter, ComponentKind::IsSquirter);
    ADD_IF_HAS(CanBeHeld_HT, ComponentKind::CanBeHeld_HT);
    ADD_IF_HAS(CanBeHeld, ComponentKind::CanBeHeld);
    ADD_IF_HAS(BypassAutomationState, ComponentKind::BypassAutomationState);
    ADD_IF_HAS(AICloseTab, ComponentKind::AICloseTab);
    ADD_IF_HAS(AIPlayJukebox, ComponentKind::AIPlayJukebox);
    ADD_IF_HAS(AIWaitInQueue, ComponentKind::AIWaitInQueue);
    ADD_IF_HAS(AIDrinking, ComponentKind::AIDrinking);
    ADD_IF_HAS(AIUseBathroom, ComponentKind::AIUseBathroom);
    ADD_IF_HAS(AIWandering, ComponentKind::AIWandering);
    ADD_IF_HAS(AICleanVomit, ComponentKind::AICleanVomit);

#undef ADD_IF_HAS
    return out;
}

[[nodiscard]] bool apply_component_blob(afterhours::Entity& e,
                                       const ComponentBlobDTO& b) {
    switch (b.kind) {
        case ComponentKind::Transform: {
            auto& cmp = e.addComponent<Transform>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::HasName: {
            auto& cmp = e.addComponent<HasName>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::CanHoldItem: {
            auto& cmp = e.addComponent<CanHoldItem>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::SimpleColoredBoxRenderer: {
            auto& cmp = e.addComponent<SimpleColoredBoxRenderer>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::CanBeHighlighted: {
            auto& cmp = e.addComponent<CanBeHighlighted>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::CanHighlightOthers: {
            auto& cmp = e.addComponent<CanHighlightOthers>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::CanHoldFurniture: {
            auto& cmp = e.addComponent<CanHoldFurniture>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::CanBeGhostPlayer: {
            auto& cmp = e.addComponent<CanBeGhostPlayer>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::CanPerformJob: {
            auto& cmp = e.addComponent<CanPerformJob>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::ModelRenderer: {
            auto& cmp = e.addComponent<ModelRenderer>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::CanBePushed: {
            auto& cmp = e.addComponent<CanBePushed>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::CustomHeldItemPosition: {
            auto& cmp = e.addComponent<CustomHeldItemPosition>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::HasWork: {
            auto& cmp = e.addComponent<HasWork>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::HasBaseSpeed: {
            auto& cmp = e.addComponent<HasBaseSpeed>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::IsSolid: {
            auto& cmp = e.addComponent<IsSolid>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::HasPatience: {
            auto& cmp = e.addComponent<HasPatience>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::HasProgression: {
            auto& cmp = e.addComponent<HasProgression>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::IsRotatable: {
            auto& cmp = e.addComponent<IsRotatable>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::CanGrabFromOtherFurniture: {
            auto& cmp = e.addComponent<CanGrabFromOtherFurniture>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::ConveysHeldItem: {
            auto& cmp = e.addComponent<ConveysHeldItem>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::HasWaitingQueue: {
            auto& cmp = e.addComponent<HasWaitingQueue>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::CanBeTakenFrom: {
            auto& cmp = e.addComponent<CanBeTakenFrom>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::IsItemContainer: {
            auto& cmp = e.addComponent<IsItemContainer>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::UsesCharacterModel: {
            auto& cmp = e.addComponent<UsesCharacterModel>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::HasDynamicModelName: {
            auto& cmp = e.addComponent<HasDynamicModelName>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::IsTriggerArea: {
            auto& cmp = e.addComponent<IsTriggerArea>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::HasSpeechBubble: {
            auto& cmp = e.addComponent<HasSpeechBubble>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::Indexer: {
            auto& cmp = e.addComponent<Indexer>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::IsSpawner: {
            auto& cmp = e.addComponent<IsSpawner>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::HasRopeToItem: {
            auto& cmp = e.addComponent<HasRopeToItem>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::HasSubtype: {
            auto& cmp = e.addComponent<HasSubtype>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::IsItem: {
            auto& cmp = e.addComponent<IsItem>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::IsDrink: {
            auto& cmp = e.addComponent<IsDrink>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::AddsIngredient: {
            auto& cmp = e.addComponent<AddsIngredient>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::CanOrderDrink: {
            auto& cmp = e.addComponent<CanOrderDrink>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::IsPnumaticPipe: {
            auto& cmp = e.addComponent<IsPnumaticPipe>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::IsProgressionManager: {
            auto& cmp = e.addComponent<IsProgressionManager>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::IsFloorMarker: {
            auto& cmp = e.addComponent<IsFloorMarker>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::IsBank: {
            auto& cmp = e.addComponent<IsBank>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::IsFreeInStore: {
            auto& cmp = e.addComponent<IsFreeInStore>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::IsToilet: {
            auto& cmp = e.addComponent<IsToilet>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::CanPathfind: {
            auto& cmp = e.addComponent<CanPathfind>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::IsRoundSettingsManager: {
            auto& cmp = e.addComponent<IsRoundSettingsManager>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::HasFishingGame: {
            auto& cmp = e.addComponent<HasFishingGame>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::IsStoreSpawned: {
            auto& cmp = e.addComponent<IsStoreSpawned>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::HasLastInteractedCustomer: {
            auto& cmp = e.addComponent<HasLastInteractedCustomer>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::CanChangeSettingsInteractively: {
            auto& cmp = e.addComponent<CanChangeSettingsInteractively>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::IsNuxManager: {
            auto& cmp = e.addComponent<IsNuxManager>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::IsNux: {
            auto& cmp = e.addComponent<IsNux>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::CollectsUserInput: {
            auto& cmp = e.addComponent<CollectsUserInput>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::IsSnappable: {
            auto& cmp = e.addComponent<IsSnappable>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::HasClientID: {
            auto& cmp = e.addComponent<HasClientID>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::RespondsToUserInput: {
            auto& cmp = e.addComponent<RespondsToUserInput>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::CanHoldHandTruck: {
            auto& cmp = e.addComponent<CanHoldHandTruck>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::RespondsToDayNight: {
            auto& cmp = e.addComponent<RespondsToDayNight>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::HasDayNightTimer: {
            auto& cmp = e.addComponent<HasDayNightTimer>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::CollectsCustomerFeedback: {
            auto& cmp = e.addComponent<CollectsCustomerFeedback>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::IsSquirter: {
            auto& cmp = e.addComponent<IsSquirter>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::CanBeHeld_HT: {
            auto& cmp = e.addComponent<CanBeHeld_HT>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::CanBeHeld: {
            auto& cmp = e.addComponent<CanBeHeld>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::BypassAutomationState: {
            auto& cmp = e.addComponent<BypassAutomationState>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::AICloseTab: {
            auto& cmp = e.addComponent<AICloseTab>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::AIPlayJukebox: {
            auto& cmp = e.addComponent<AIPlayJukebox>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::AIWaitInQueue: {
            auto& cmp = e.addComponent<AIWaitInQueue>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::AIDrinking: {
            auto& cmp = e.addComponent<AIDrinking>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::AIUseBathroom: {
            auto& cmp = e.addComponent<AIUseBathroom>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::AIWandering: {
            auto& cmp = e.addComponent<AIWandering>();
            return decode_component_into(cmp, b.bytes);
        }
        case ComponentKind::AICleanVomit: {
            auto& cmp = e.addComponent<AICleanVomit>();
            return decode_component_into(cmp, b.bytes);
        }
    }
    return false;
}

[[nodiscard]] bool apply_snapshot_to_entity(afterhours::Entity& e,
                                           const EntitySnapshotDTO& s) {
    clear_all_components(e);
    e.id = s.id;
    e.entity_type = s.entity_type;
    e.tags = s.tags;
    e.cleanup = s.cleanup;

    for (const auto& b : s.components) {
        if (!apply_component_blob(e, b)) {
            log_warn("snapshot_blob: failed applying component kind {} on id={}",
                     static_cast<int>(b.kind), e.id);
            return false;
        }
    }
    return true;
}

template<typename T>
[[nodiscard]] std::optional<T> deserialize_one_object(const std::string& buf) {
    TContext ctx{};
    Deserializer des{ctx, buf.begin(), buf.size()};
    T obj{};
    des.object(obj);
    if (des.adapter().error() != bitsery::ReaderError::NoError) {
        return std::nullopt;
    }
    return obj;
}

}  // namespace

std::string encode_entity(const afterhours::Entity& entity) {
    EntitySnapshotDTO dto = snapshot_entity(entity);

    Buffer buffer;
    TContext ctx{};
    Serializer ser{ctx, buffer};
    ser.object(dto);
    ser.adapter().flush();
    return buffer;
}

bool decode_into_entity(afterhours::Entity& entity, const std::string& blob) {
    auto dto_opt = deserialize_one_object<EntitySnapshotDTO>(blob);
    if (!dto_opt.has_value()) return false;
    return apply_snapshot_to_entity(entity, *dto_opt);
}

std::string encode_current_world() {
    WorldSnapshotDTO world;
    world.entities.reserve(EntityHelper::get_entities().size());
    for (const auto& sp : EntityHelper::get_entities()) {
        if (!sp) continue;
        world.entities.push_back(snapshot_entity(*sp));
    }

    Buffer buffer;
    TContext ctx{};
    Serializer ser{ctx, buffer};
    ser.object(world);
    ser.adapter().flush();
    return buffer;
}

bool decode_into_current_world(const std::string& blob) {
    auto world_opt = deserialize_one_object<WorldSnapshotDTO>(blob);
    if (!world_opt.has_value()) return false;
    const WorldSnapshotDTO& world = *world_opt;
    if (world.version != 1) {
        log_warn("snapshot_blob: unsupported world snapshot version {}",
                 world.version);
        return false;
    }

    Entities new_entities;
    new_entities.reserve(world.entities.size());

    for (const auto& ent_dto : world.entities) {
        std::shared_ptr<afterhours::Entity> sp(new afterhours::Entity());
        if (!apply_snapshot_to_entity(*sp, ent_dto)) return false;
        new_entities.push_back(std::move(sp));
    }

    EntityHelper::get_current_collection().replace_all_entities(
        std::move(new_entities));
    return true;
}

}  // namespace snapshot_blob

