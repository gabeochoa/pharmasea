#include "world_snapshot_blob.h"

#include "../bitsery_include.h"
#include "../engine/log.h"
#include "../entity_helper.h"
#include "../network/polymorphic_components.h"  // brings in all component types + bitsery config

#include "afterhours/src/core/base_component.h"

#include <bitsery/ext/std_bitset.h>
#include <bitsery/ext/std_variant.h>

namespace snapshot_blob {

namespace {
using Buffer = std::string;
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;
using TContext = std::tuple<>;
using Serializer = bitsery::Serializer<OutputAdapter, TContext>;
using Deserializer = bitsery::Deserializer<InputAdapter, TContext>;

// NOTE: This list should include every component type that is intended to be
// persisted in the full snapshot payload (Phase 1 style).
//
// Keeping it in one macro lets us use it for:
// - the variant type list
// - the variant serializer visitors
// - the "if has<T>()" snapshot gather loop
#define PHARMASEA_SNAPSHOT_COMPONENT_VARIANT_TYPES                               \
    Transform, HasName, CanHoldItem, SimpleColoredBoxRenderer, CanBeHighlighted, \
        CanHighlightOthers, CanHoldFurniture, CanBeGhostPlayer, CanPerformJob,   \
        ModelRenderer, CanBePushed, CustomHeldItemPosition, HasWork,             \
        HasBaseSpeed, IsSolid, HasPatience, HasProgression, IsRotatable,         \
        CanGrabFromOtherFurniture, ConveysHeldItem, HasWaitingQueue,             \
        CanBeTakenFrom, IsItemContainer, UsesCharacterModel, HasDynamicModelName,\
        IsTriggerArea, HasSpeechBubble, Indexer, IsSpawner, HasRopeToItem,       \
        HasSubtype, IsItem, IsDrink, AddsIngredient, CanOrderDrink,             \
        IsPnumaticPipe, IsProgressionManager, IsFloorMarker, IsBank,             \
        IsFreeInStore, IsToilet, CanPathfind, IsRoundSettingsManager,            \
        HasFishingGame, IsStoreSpawned, HasLastInteractedCustomer,              \
        CanChangeSettingsInteractively, IsNuxManager, IsNux, CollectsUserInput, \
        IsSnappable, HasClientID, RespondsToUserInput, CanHoldHandTruck,        \
        RespondsToDayNight, HasDayNightTimer, CollectsCustomerFeedback,         \
        IsSquirter, CanBeHeld_HT, CanBeHeld, BypassAutomationState,             \
        AICloseTab, AIPlayJukebox, AIWaitInQueue, AIDrinking, AIUseBathroom,    \
        AIWandering, AICleanVomit

using ComponentVariant = std::variant<PHARMASEA_SNAPSHOT_COMPONENT_VARIANT_TYPES>;

template<typename S>
void serialize_component_variant(S& s, ComponentVariant& v) {
    // Keep order aligned with ComponentVariant's alternatives.
    s.ext(v,
          bitsery::ext::StdVariant{
              [](S& sv, Transform& o) { sv.object(o); },
              [](S& sv, HasName& o) { sv.object(o); },
              [](S& sv, CanHoldItem& o) { sv.object(o); },
              [](S& sv, SimpleColoredBoxRenderer& o) { sv.object(o); },
              [](S& sv, CanBeHighlighted& o) { sv.object(o); },
              [](S& sv, CanHighlightOthers& o) { sv.object(o); },
              [](S& sv, CanHoldFurniture& o) { sv.object(o); },
              [](S& sv, CanBeGhostPlayer& o) { sv.object(o); },
              [](S& sv, CanPerformJob& o) { sv.object(o); },
              [](S& sv, ModelRenderer& o) { sv.object(o); },
              [](S& sv, CanBePushed& o) { sv.object(o); },
              [](S& sv, CustomHeldItemPosition& o) { sv.object(o); },
              [](S& sv, HasWork& o) { sv.object(o); },
              [](S& sv, HasBaseSpeed& o) { sv.object(o); },
              [](S& sv, IsSolid& o) { sv.object(o); },
              [](S& sv, HasPatience& o) { sv.object(o); },
              [](S& sv, HasProgression& o) { sv.object(o); },
              [](S& sv, IsRotatable& o) { sv.object(o); },
              [](S& sv, CanGrabFromOtherFurniture& o) { sv.object(o); },
              [](S& sv, ConveysHeldItem& o) { sv.object(o); },
              [](S& sv, HasWaitingQueue& o) { sv.object(o); },
              [](S& sv, CanBeTakenFrom& o) { sv.object(o); },
              [](S& sv, IsItemContainer& o) { sv.object(o); },
              [](S& sv, UsesCharacterModel& o) { sv.object(o); },
              [](S& sv, HasDynamicModelName& o) { sv.object(o); },
              [](S& sv, IsTriggerArea& o) { sv.object(o); },
              [](S& sv, HasSpeechBubble& o) { sv.object(o); },
              [](S& sv, Indexer& o) { sv.object(o); },
              [](S& sv, IsSpawner& o) { sv.object(o); },
              [](S& sv, HasRopeToItem& o) { sv.object(o); },
              [](S& sv, HasSubtype& o) { sv.object(o); },
              [](S& sv, IsItem& o) { sv.object(o); },
              [](S& sv, IsDrink& o) { sv.object(o); },
              [](S& sv, AddsIngredient& o) { sv.object(o); },
              [](S& sv, CanOrderDrink& o) { sv.object(o); },
              [](S& sv, IsPnumaticPipe& o) { sv.object(o); },
              [](S& sv, IsProgressionManager& o) { sv.object(o); },
              [](S& sv, IsFloorMarker& o) { sv.object(o); },
              [](S& sv, IsBank& o) { sv.object(o); },
              [](S& sv, IsFreeInStore& o) { sv.object(o); },
              [](S& sv, IsToilet& o) { sv.object(o); },
              [](S& sv, CanPathfind& o) { sv.object(o); },
              [](S& sv, IsRoundSettingsManager& o) { sv.object(o); },
              [](S& sv, HasFishingGame& o) { sv.object(o); },
              [](S& sv, IsStoreSpawned& o) { sv.object(o); },
              [](S& sv, HasLastInteractedCustomer& o) { sv.object(o); },
              [](S& sv, CanChangeSettingsInteractively& o) { sv.object(o); },
              [](S& sv, IsNuxManager& o) { sv.object(o); },
              [](S& sv, IsNux& o) { sv.object(o); },
              [](S& sv, CollectsUserInput& o) { sv.object(o); },
              [](S& sv, IsSnappable& o) { sv.object(o); },
              [](S& sv, HasClientID& o) { sv.object(o); },
              [](S& sv, RespondsToUserInput& o) { sv.object(o); },
              [](S& sv, CanHoldHandTruck& o) { sv.object(o); },
              [](S& sv, RespondsToDayNight& o) { sv.object(o); },
              [](S& sv, HasDayNightTimer& o) { sv.object(o); },
              [](S& sv, CollectsCustomerFeedback& o) { sv.object(o); },
              [](S& sv, IsSquirter& o) { sv.object(o); },
              [](S& sv, CanBeHeld_HT& o) { sv.object(o); },
              [](S& sv, CanBeHeld& o) { sv.object(o); },
              [](S& sv, BypassAutomationState& o) { sv.object(o); },
              [](S& sv, AICloseTab& o) { sv.object(o); },
              [](S& sv, AIPlayJukebox& o) { sv.object(o); },
              [](S& sv, AIWaitInQueue& o) { sv.object(o); },
              [](S& sv, AIDrinking& o) { sv.object(o); },
              [](S& sv, AIUseBathroom& o) { sv.object(o); },
              [](S& sv, AIWandering& o) { sv.object(o); },
              [](S& sv, AICleanVomit& o) { sv.object(o); },
          });
}

struct EntitySnapshotDTO {
    afterhours::EntityID id = -1;
    int entity_type = 0;
    afterhours::TagBitset tags{};
    bool cleanup = false;

    std::vector<ComponentVariant> components;

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

        for (uint8_t i = 0; i < num_components; ++i) {
            serialize_component_variant(s, components[i]);
        }
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
#define ADD_IF_HAS(T) \
    do {              \
        if (nc.has<T>()) out.components.emplace_back(nc.get<T>()); \
    } while (0)

    ADD_IF_HAS(Transform);
    ADD_IF_HAS(HasName);
    ADD_IF_HAS(CanHoldItem);
    ADD_IF_HAS(SimpleColoredBoxRenderer);
    ADD_IF_HAS(CanBeHighlighted);
    ADD_IF_HAS(CanHighlightOthers);
    ADD_IF_HAS(CanHoldFurniture);
    ADD_IF_HAS(CanBeGhostPlayer);
    ADD_IF_HAS(CanPerformJob);
    ADD_IF_HAS(ModelRenderer);
    ADD_IF_HAS(CanBePushed);
    ADD_IF_HAS(CustomHeldItemPosition);
    ADD_IF_HAS(HasWork);
    ADD_IF_HAS(HasBaseSpeed);
    ADD_IF_HAS(IsSolid);
    ADD_IF_HAS(HasPatience);
    ADD_IF_HAS(HasProgression);
    ADD_IF_HAS(IsRotatable);
    ADD_IF_HAS(CanGrabFromOtherFurniture);
    ADD_IF_HAS(ConveysHeldItem);
    ADD_IF_HAS(HasWaitingQueue);
    ADD_IF_HAS(CanBeTakenFrom);
    ADD_IF_HAS(IsItemContainer);
    ADD_IF_HAS(UsesCharacterModel);
    ADD_IF_HAS(HasDynamicModelName);
    ADD_IF_HAS(IsTriggerArea);
    ADD_IF_HAS(HasSpeechBubble);
    ADD_IF_HAS(Indexer);
    ADD_IF_HAS(IsSpawner);
    ADD_IF_HAS(HasRopeToItem);
    ADD_IF_HAS(HasSubtype);
    ADD_IF_HAS(IsItem);
    ADD_IF_HAS(IsDrink);
    ADD_IF_HAS(AddsIngredient);
    ADD_IF_HAS(CanOrderDrink);
    ADD_IF_HAS(IsPnumaticPipe);
    ADD_IF_HAS(IsProgressionManager);
    ADD_IF_HAS(IsFloorMarker);
    ADD_IF_HAS(IsBank);
    ADD_IF_HAS(IsFreeInStore);
    ADD_IF_HAS(IsToilet);
    ADD_IF_HAS(CanPathfind);
    ADD_IF_HAS(IsRoundSettingsManager);
    ADD_IF_HAS(HasFishingGame);
    ADD_IF_HAS(IsStoreSpawned);
    ADD_IF_HAS(HasLastInteractedCustomer);
    ADD_IF_HAS(CanChangeSettingsInteractively);
    ADD_IF_HAS(IsNuxManager);
    ADD_IF_HAS(IsNux);
    ADD_IF_HAS(CollectsUserInput);
    ADD_IF_HAS(IsSnappable);
    ADD_IF_HAS(HasClientID);
    ADD_IF_HAS(RespondsToUserInput);
    ADD_IF_HAS(CanHoldHandTruck);
    ADD_IF_HAS(RespondsToDayNight);
    ADD_IF_HAS(HasDayNightTimer);
    ADD_IF_HAS(CollectsCustomerFeedback);
    ADD_IF_HAS(IsSquirter);
    ADD_IF_HAS(CanBeHeld_HT);
    ADD_IF_HAS(CanBeHeld);
    ADD_IF_HAS(BypassAutomationState);
    ADD_IF_HAS(AICloseTab);
    ADD_IF_HAS(AIPlayJukebox);
    ADD_IF_HAS(AIWaitInQueue);
    ADD_IF_HAS(AIDrinking);
    ADD_IF_HAS(AIUseBathroom);
    ADD_IF_HAS(AIWandering);
    ADD_IF_HAS(AICleanVomit);

#undef ADD_IF_HAS
    return out;
}

void apply_snapshot_to_entity(afterhours::Entity& e, const EntitySnapshotDTO& s) {
    clear_all_components(e);
    e.id = s.id;
    e.entity_type = s.entity_type;
    e.tags = s.tags;
    e.cleanup = s.cleanup;

    for (const auto& cv : s.components) {
        std::visit(
            [&](auto&& value) {
                using T = std::decay_t<decltype(value)>;
                // Ensure the component exists, then overwrite its serialized
                // state. (Fields not serialized remain defaulted.)
                T& dst = e.addComponent<T>();
                dst = value;
            },
            cv);
    }
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
    apply_snapshot_to_entity(entity, *dto_opt);
    return true;
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
        apply_snapshot_to_entity(*sp, ent_dto);
        new_entities.push_back(std::move(sp));
    }

    EntityHelper::get_current_collection().replace_all_entities(
        std::move(new_entities));
    return true;
}

}  // namespace snapshot_blob

