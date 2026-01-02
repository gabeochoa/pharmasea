#include "world_snapshot_blob.h"

#include "../bitsery_include.h"
#include "../engine/log.h"
#include "../entity_helper.h"
#include "../network/polymorphic_components.h"  // includes all component types + bitsery wiring

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
// IMPORTANT: never renumber; only append.
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

inline void clear_all_components(afterhours::Entity& e) {
    e.componentSet.reset();
    for (auto& ptr : e.componentArray) ptr.reset();
}

template<typename E>
inline void write_kind(E& s, ComponentKind k) {
    std::uint16_t raw = static_cast<std::uint16_t>(k);
    s.value2b(raw);
}

template<typename S>
inline bool read_kind(S& s, ComponentKind& out) {
    std::uint16_t raw = 0;
    s.value2b(raw);
    out = static_cast<ComponentKind>(raw);
    return s.adapter().error() == bitsery::ReaderError::NoError;
}

template<typename T>
inline void count_if_has(const afterhours::Entity& e, uint8_t& count) {
    auto& nc = const_cast<afterhours::Entity&>(e);
    if (nc.has<T>()) ++count;
}

template<typename T>
inline void write_if_has(Serializer& s, afterhours::Entity& e, ComponentKind k) {
    if (!e.has<T>()) return;
    write_kind(s, k);
    auto& cmp = e.get<T>();
    s.object(cmp);
}

[[nodiscard]] uint8_t count_components(const afterhours::Entity& e) {
    uint8_t c = 0;
    count_if_has<Transform>(e, c);
    count_if_has<HasName>(e, c);
    count_if_has<CanHoldItem>(e, c);
    count_if_has<SimpleColoredBoxRenderer>(e, c);
    count_if_has<CanBeHighlighted>(e, c);
    count_if_has<CanHighlightOthers>(e, c);
    count_if_has<CanHoldFurniture>(e, c);
    count_if_has<CanBeGhostPlayer>(e, c);
    count_if_has<CanPerformJob>(e, c);
    count_if_has<ModelRenderer>(e, c);
    count_if_has<CanBePushed>(e, c);
    count_if_has<CustomHeldItemPosition>(e, c);
    count_if_has<HasWork>(e, c);
    count_if_has<HasBaseSpeed>(e, c);
    count_if_has<IsSolid>(e, c);
    count_if_has<HasPatience>(e, c);
    count_if_has<HasProgression>(e, c);
    count_if_has<IsRotatable>(e, c);
    count_if_has<CanGrabFromOtherFurniture>(e, c);
    count_if_has<ConveysHeldItem>(e, c);
    count_if_has<HasWaitingQueue>(e, c);
    count_if_has<CanBeTakenFrom>(e, c);
    count_if_has<IsItemContainer>(e, c);
    count_if_has<UsesCharacterModel>(e, c);
    count_if_has<HasDynamicModelName>(e, c);
    count_if_has<IsTriggerArea>(e, c);
    count_if_has<HasSpeechBubble>(e, c);
    count_if_has<Indexer>(e, c);
    count_if_has<IsSpawner>(e, c);
    count_if_has<HasRopeToItem>(e, c);
    count_if_has<HasSubtype>(e, c);
    count_if_has<IsItem>(e, c);
    count_if_has<IsDrink>(e, c);
    count_if_has<AddsIngredient>(e, c);
    count_if_has<CanOrderDrink>(e, c);
    count_if_has<IsPnumaticPipe>(e, c);
    count_if_has<IsProgressionManager>(e, c);
    count_if_has<IsFloorMarker>(e, c);
    count_if_has<IsBank>(e, c);
    count_if_has<IsFreeInStore>(e, c);
    count_if_has<IsToilet>(e, c);
    count_if_has<CanPathfind>(e, c);
    count_if_has<IsRoundSettingsManager>(e, c);
    count_if_has<HasFishingGame>(e, c);
    count_if_has<IsStoreSpawned>(e, c);
    count_if_has<HasLastInteractedCustomer>(e, c);
    count_if_has<CanChangeSettingsInteractively>(e, c);
    count_if_has<IsNuxManager>(e, c);
    count_if_has<IsNux>(e, c);
    count_if_has<CollectsUserInput>(e, c);
    count_if_has<IsSnappable>(e, c);
    count_if_has<HasClientID>(e, c);
    count_if_has<RespondsToUserInput>(e, c);
    count_if_has<CanHoldHandTruck>(e, c);
    count_if_has<RespondsToDayNight>(e, c);
    count_if_has<HasDayNightTimer>(e, c);
    count_if_has<CollectsCustomerFeedback>(e, c);
    count_if_has<IsSquirter>(e, c);
    count_if_has<CanBeHeld_HT>(e, c);
    count_if_has<CanBeHeld>(e, c);
    count_if_has<BypassAutomationState>(e, c);
    count_if_has<AICloseTab>(e, c);
    count_if_has<AIPlayJukebox>(e, c);
    count_if_has<AIWaitInQueue>(e, c);
    count_if_has<AIDrinking>(e, c);
    count_if_has<AIUseBathroom>(e, c);
    count_if_has<AIWandering>(e, c);
    count_if_has<AICleanVomit>(e, c);
    return c;
}

void write_entity(Serializer& s, afterhours::Entity& e) {
    s.value4b(e.id);
    s.value4b(e.entity_type);
    s.ext(e.tags, bitsery::ext::StdBitset{});
    s.value1b(e.cleanup);

    const uint8_t count = count_components(e);
    s.value1b(count);

    write_if_has<Transform>(s, e, ComponentKind::Transform);
    write_if_has<HasName>(s, e, ComponentKind::HasName);
    write_if_has<CanHoldItem>(s, e, ComponentKind::CanHoldItem);
    write_if_has<SimpleColoredBoxRenderer>(s, e,
                                          ComponentKind::SimpleColoredBoxRenderer);
    write_if_has<CanBeHighlighted>(s, e, ComponentKind::CanBeHighlighted);
    write_if_has<CanHighlightOthers>(s, e, ComponentKind::CanHighlightOthers);
    write_if_has<CanHoldFurniture>(s, e, ComponentKind::CanHoldFurniture);
    write_if_has<CanBeGhostPlayer>(s, e, ComponentKind::CanBeGhostPlayer);
    write_if_has<CanPerformJob>(s, e, ComponentKind::CanPerformJob);
    write_if_has<ModelRenderer>(s, e, ComponentKind::ModelRenderer);
    write_if_has<CanBePushed>(s, e, ComponentKind::CanBePushed);
    write_if_has<CustomHeldItemPosition>(s, e,
                                         ComponentKind::CustomHeldItemPosition);
    write_if_has<HasWork>(s, e, ComponentKind::HasWork);
    write_if_has<HasBaseSpeed>(s, e, ComponentKind::HasBaseSpeed);
    write_if_has<IsSolid>(s, e, ComponentKind::IsSolid);
    write_if_has<HasPatience>(s, e, ComponentKind::HasPatience);
    write_if_has<HasProgression>(s, e, ComponentKind::HasProgression);
    write_if_has<IsRotatable>(s, e, ComponentKind::IsRotatable);
    write_if_has<CanGrabFromOtherFurniture>(s, e,
                                           ComponentKind::CanGrabFromOtherFurniture);
    write_if_has<ConveysHeldItem>(s, e, ComponentKind::ConveysHeldItem);
    write_if_has<HasWaitingQueue>(s, e, ComponentKind::HasWaitingQueue);
    write_if_has<CanBeTakenFrom>(s, e, ComponentKind::CanBeTakenFrom);
    write_if_has<IsItemContainer>(s, e, ComponentKind::IsItemContainer);
    write_if_has<UsesCharacterModel>(s, e, ComponentKind::UsesCharacterModel);
    write_if_has<HasDynamicModelName>(s, e, ComponentKind::HasDynamicModelName);
    write_if_has<IsTriggerArea>(s, e, ComponentKind::IsTriggerArea);
    write_if_has<HasSpeechBubble>(s, e, ComponentKind::HasSpeechBubble);
    write_if_has<Indexer>(s, e, ComponentKind::Indexer);
    write_if_has<IsSpawner>(s, e, ComponentKind::IsSpawner);
    write_if_has<HasRopeToItem>(s, e, ComponentKind::HasRopeToItem);
    write_if_has<HasSubtype>(s, e, ComponentKind::HasSubtype);
    write_if_has<IsItem>(s, e, ComponentKind::IsItem);
    write_if_has<IsDrink>(s, e, ComponentKind::IsDrink);
    write_if_has<AddsIngredient>(s, e, ComponentKind::AddsIngredient);
    write_if_has<CanOrderDrink>(s, e, ComponentKind::CanOrderDrink);
    write_if_has<IsPnumaticPipe>(s, e, ComponentKind::IsPnumaticPipe);
    write_if_has<IsProgressionManager>(s, e, ComponentKind::IsProgressionManager);
    write_if_has<IsFloorMarker>(s, e, ComponentKind::IsFloorMarker);
    write_if_has<IsBank>(s, e, ComponentKind::IsBank);
    write_if_has<IsFreeInStore>(s, e, ComponentKind::IsFreeInStore);
    write_if_has<IsToilet>(s, e, ComponentKind::IsToilet);
    write_if_has<CanPathfind>(s, e, ComponentKind::CanPathfind);
    write_if_has<IsRoundSettingsManager>(s, e,
                                        ComponentKind::IsRoundSettingsManager);
    write_if_has<HasFishingGame>(s, e, ComponentKind::HasFishingGame);
    write_if_has<IsStoreSpawned>(s, e, ComponentKind::IsStoreSpawned);
    write_if_has<HasLastInteractedCustomer>(s, e,
                                           ComponentKind::HasLastInteractedCustomer);
    write_if_has<CanChangeSettingsInteractively>(s, e,
                                                ComponentKind::CanChangeSettingsInteractively);
    write_if_has<IsNuxManager>(s, e, ComponentKind::IsNuxManager);
    write_if_has<IsNux>(s, e, ComponentKind::IsNux);
    write_if_has<CollectsUserInput>(s, e, ComponentKind::CollectsUserInput);
    write_if_has<IsSnappable>(s, e, ComponentKind::IsSnappable);
    write_if_has<HasClientID>(s, e, ComponentKind::HasClientID);
    write_if_has<RespondsToUserInput>(s, e, ComponentKind::RespondsToUserInput);
    write_if_has<CanHoldHandTruck>(s, e, ComponentKind::CanHoldHandTruck);
    write_if_has<RespondsToDayNight>(s, e, ComponentKind::RespondsToDayNight);
    write_if_has<HasDayNightTimer>(s, e, ComponentKind::HasDayNightTimer);
    write_if_has<CollectsCustomerFeedback>(s, e,
                                          ComponentKind::CollectsCustomerFeedback);
    write_if_has<IsSquirter>(s, e, ComponentKind::IsSquirter);
    write_if_has<CanBeHeld_HT>(s, e, ComponentKind::CanBeHeld_HT);
    write_if_has<CanBeHeld>(s, e, ComponentKind::CanBeHeld);
    write_if_has<BypassAutomationState>(s, e, ComponentKind::BypassAutomationState);
    write_if_has<AICloseTab>(s, e, ComponentKind::AICloseTab);
    write_if_has<AIPlayJukebox>(s, e, ComponentKind::AIPlayJukebox);
    write_if_has<AIWaitInQueue>(s, e, ComponentKind::AIWaitInQueue);
    write_if_has<AIDrinking>(s, e, ComponentKind::AIDrinking);
    write_if_has<AIUseBathroom>(s, e, ComponentKind::AIUseBathroom);
    write_if_has<AIWandering>(s, e, ComponentKind::AIWandering);
    write_if_has<AICleanVomit>(s, e, ComponentKind::AICleanVomit);
}

[[nodiscard]] bool read_entity(Deserializer& d, afterhours::Entity& e) {
    clear_all_components(e);

    d.value4b(e.id);
    d.value4b(e.entity_type);
    d.ext(e.tags, bitsery::ext::StdBitset{});
    d.value1b(e.cleanup);

    uint8_t count = 0;
    d.value1b(count);
    if (d.adapter().error() != bitsery::ReaderError::NoError) return false;

    for (uint8_t i = 0; i < count; ++i) {
        ComponentKind kind{};
        if (!read_kind(d, kind)) return false;

        switch (kind) {
            case ComponentKind::Transform: {
                auto& cmp = e.addComponent<Transform>();
                d.object(cmp);
            } break;
            case ComponentKind::HasName: {
                auto& cmp = e.addComponent<HasName>();
                d.object(cmp);
            } break;
            case ComponentKind::CanHoldItem: {
                auto& cmp = e.addComponent<CanHoldItem>();
                d.object(cmp);
            } break;
            case ComponentKind::SimpleColoredBoxRenderer: {
                auto& cmp = e.addComponent<SimpleColoredBoxRenderer>();
                d.object(cmp);
            } break;
            case ComponentKind::CanBeHighlighted: {
                auto& cmp = e.addComponent<CanBeHighlighted>();
                d.object(cmp);
            } break;
            case ComponentKind::CanHighlightOthers: {
                auto& cmp = e.addComponent<CanHighlightOthers>();
                d.object(cmp);
            } break;
            case ComponentKind::CanHoldFurniture: {
                auto& cmp = e.addComponent<CanHoldFurniture>();
                d.object(cmp);
            } break;
            case ComponentKind::CanBeGhostPlayer: {
                auto& cmp = e.addComponent<CanBeGhostPlayer>();
                d.object(cmp);
            } break;
            case ComponentKind::CanPerformJob: {
                auto& cmp = e.addComponent<CanPerformJob>();
                d.object(cmp);
            } break;
            case ComponentKind::ModelRenderer: {
                auto& cmp = e.addComponent<ModelRenderer>();
                d.object(cmp);
            } break;
            case ComponentKind::CanBePushed: {
                auto& cmp = e.addComponent<CanBePushed>();
                d.object(cmp);
            } break;
            case ComponentKind::CustomHeldItemPosition: {
                auto& cmp = e.addComponent<CustomHeldItemPosition>();
                d.object(cmp);
            } break;
            case ComponentKind::HasWork: {
                auto& cmp = e.addComponent<HasWork>();
                d.object(cmp);
            } break;
            case ComponentKind::HasBaseSpeed: {
                auto& cmp = e.addComponent<HasBaseSpeed>();
                d.object(cmp);
            } break;
            case ComponentKind::IsSolid: {
                auto& cmp = e.addComponent<IsSolid>();
                d.object(cmp);
            } break;
            case ComponentKind::HasPatience: {
                auto& cmp = e.addComponent<HasPatience>();
                d.object(cmp);
            } break;
            case ComponentKind::HasProgression: {
                auto& cmp = e.addComponent<HasProgression>();
                d.object(cmp);
            } break;
            case ComponentKind::IsRotatable: {
                auto& cmp = e.addComponent<IsRotatable>();
                d.object(cmp);
            } break;
            case ComponentKind::CanGrabFromOtherFurniture: {
                auto& cmp = e.addComponent<CanGrabFromOtherFurniture>();
                d.object(cmp);
            } break;
            case ComponentKind::ConveysHeldItem: {
                auto& cmp = e.addComponent<ConveysHeldItem>();
                d.object(cmp);
            } break;
            case ComponentKind::HasWaitingQueue: {
                auto& cmp = e.addComponent<HasWaitingQueue>();
                d.object(cmp);
            } break;
            case ComponentKind::CanBeTakenFrom: {
                auto& cmp = e.addComponent<CanBeTakenFrom>();
                d.object(cmp);
            } break;
            case ComponentKind::IsItemContainer: {
                auto& cmp = e.addComponent<IsItemContainer>();
                d.object(cmp);
            } break;
            case ComponentKind::UsesCharacterModel: {
                auto& cmp = e.addComponent<UsesCharacterModel>();
                d.object(cmp);
            } break;
            case ComponentKind::HasDynamicModelName: {
                auto& cmp = e.addComponent<HasDynamicModelName>();
                d.object(cmp);
            } break;
            case ComponentKind::IsTriggerArea: {
                auto& cmp = e.addComponent<IsTriggerArea>();
                d.object(cmp);
            } break;
            case ComponentKind::HasSpeechBubble: {
                auto& cmp = e.addComponent<HasSpeechBubble>();
                d.object(cmp);
            } break;
            case ComponentKind::Indexer: {
                auto& cmp = e.addComponent<Indexer>();
                d.object(cmp);
            } break;
            case ComponentKind::IsSpawner: {
                auto& cmp = e.addComponent<IsSpawner>();
                d.object(cmp);
            } break;
            case ComponentKind::HasRopeToItem: {
                auto& cmp = e.addComponent<HasRopeToItem>();
                d.object(cmp);
            } break;
            case ComponentKind::HasSubtype: {
                auto& cmp = e.addComponent<HasSubtype>();
                d.object(cmp);
            } break;
            case ComponentKind::IsItem: {
                auto& cmp = e.addComponent<IsItem>();
                d.object(cmp);
            } break;
            case ComponentKind::IsDrink: {
                auto& cmp = e.addComponent<IsDrink>();
                d.object(cmp);
            } break;
            case ComponentKind::AddsIngredient: {
                auto& cmp = e.addComponent<AddsIngredient>();
                d.object(cmp);
            } break;
            case ComponentKind::CanOrderDrink: {
                auto& cmp = e.addComponent<CanOrderDrink>();
                d.object(cmp);
            } break;
            case ComponentKind::IsPnumaticPipe: {
                auto& cmp = e.addComponent<IsPnumaticPipe>();
                d.object(cmp);
            } break;
            case ComponentKind::IsProgressionManager: {
                auto& cmp = e.addComponent<IsProgressionManager>();
                d.object(cmp);
            } break;
            case ComponentKind::IsFloorMarker: {
                auto& cmp = e.addComponent<IsFloorMarker>();
                d.object(cmp);
            } break;
            case ComponentKind::IsBank: {
                auto& cmp = e.addComponent<IsBank>();
                d.object(cmp);
            } break;
            case ComponentKind::IsFreeInStore: {
                auto& cmp = e.addComponent<IsFreeInStore>();
                d.object(cmp);
            } break;
            case ComponentKind::IsToilet: {
                auto& cmp = e.addComponent<IsToilet>();
                d.object(cmp);
            } break;
            case ComponentKind::CanPathfind: {
                auto& cmp = e.addComponent<CanPathfind>();
                d.object(cmp);
            } break;
            case ComponentKind::IsRoundSettingsManager: {
                auto& cmp = e.addComponent<IsRoundSettingsManager>();
                d.object(cmp);
            } break;
            case ComponentKind::HasFishingGame: {
                auto& cmp = e.addComponent<HasFishingGame>();
                d.object(cmp);
            } break;
            case ComponentKind::IsStoreSpawned: {
                auto& cmp = e.addComponent<IsStoreSpawned>();
                d.object(cmp);
            } break;
            case ComponentKind::HasLastInteractedCustomer: {
                auto& cmp = e.addComponent<HasLastInteractedCustomer>();
                d.object(cmp);
            } break;
            case ComponentKind::CanChangeSettingsInteractively: {
                auto& cmp = e.addComponent<CanChangeSettingsInteractively>();
                d.object(cmp);
            } break;
            case ComponentKind::IsNuxManager: {
                auto& cmp = e.addComponent<IsNuxManager>();
                d.object(cmp);
            } break;
            case ComponentKind::IsNux: {
                auto& cmp = e.addComponent<IsNux>();
                d.object(cmp);
            } break;
            case ComponentKind::CollectsUserInput: {
                auto& cmp = e.addComponent<CollectsUserInput>();
                d.object(cmp);
            } break;
            case ComponentKind::IsSnappable: {
                auto& cmp = e.addComponent<IsSnappable>();
                d.object(cmp);
            } break;
            case ComponentKind::HasClientID: {
                auto& cmp = e.addComponent<HasClientID>();
                d.object(cmp);
            } break;
            case ComponentKind::RespondsToUserInput: {
                auto& cmp = e.addComponent<RespondsToUserInput>();
                d.object(cmp);
            } break;
            case ComponentKind::CanHoldHandTruck: {
                auto& cmp = e.addComponent<CanHoldHandTruck>();
                d.object(cmp);
            } break;
            case ComponentKind::RespondsToDayNight: {
                auto& cmp = e.addComponent<RespondsToDayNight>();
                d.object(cmp);
            } break;
            case ComponentKind::HasDayNightTimer: {
                auto& cmp = e.addComponent<HasDayNightTimer>();
                d.object(cmp);
            } break;
            case ComponentKind::CollectsCustomerFeedback: {
                auto& cmp = e.addComponent<CollectsCustomerFeedback>();
                d.object(cmp);
            } break;
            case ComponentKind::IsSquirter: {
                auto& cmp = e.addComponent<IsSquirter>();
                d.object(cmp);
            } break;
            case ComponentKind::CanBeHeld_HT: {
                auto& cmp = e.addComponent<CanBeHeld_HT>();
                d.object(cmp);
            } break;
            case ComponentKind::CanBeHeld: {
                auto& cmp = e.addComponent<CanBeHeld>();
                d.object(cmp);
            } break;
            case ComponentKind::BypassAutomationState: {
                auto& cmp = e.addComponent<BypassAutomationState>();
                d.object(cmp);
            } break;
            case ComponentKind::AICloseTab: {
                auto& cmp = e.addComponent<AICloseTab>();
                d.object(cmp);
            } break;
            case ComponentKind::AIPlayJukebox: {
                auto& cmp = e.addComponent<AIPlayJukebox>();
                d.object(cmp);
            } break;
            case ComponentKind::AIWaitInQueue: {
                auto& cmp = e.addComponent<AIWaitInQueue>();
                d.object(cmp);
            } break;
            case ComponentKind::AIDrinking: {
                auto& cmp = e.addComponent<AIDrinking>();
                d.object(cmp);
            } break;
            case ComponentKind::AIUseBathroom: {
                auto& cmp = e.addComponent<AIUseBathroom>();
                d.object(cmp);
            } break;
            case ComponentKind::AIWandering: {
                auto& cmp = e.addComponent<AIWandering>();
                d.object(cmp);
            } break;
            case ComponentKind::AICleanVomit: {
                auto& cmp = e.addComponent<AICleanVomit>();
                d.object(cmp);
            } break;
            default:
                log_warn("snapshot_blob: unknown component kind {}", (int)kind);
                return false;
        }

        if (d.adapter().error() != bitsery::ReaderError::NoError) return false;
    }

    return true;
}

}  // namespace

std::string encode_entity(const afterhours::Entity& entity) {
    Buffer buffer;
    TContext ctx{};
    Serializer ser{ctx, buffer};
    auto& e = const_cast<afterhours::Entity&>(entity);
    write_entity(ser, e);
    ser.adapter().flush();
    return buffer;
}

bool decode_into_entity(afterhours::Entity& entity, const std::string& blob) {
    TContext ctx{};
    Deserializer des{ctx, blob.begin(), blob.size()};
    const bool ok = read_entity(des, entity);
    return ok && des.adapter().error() == bitsery::ReaderError::NoError;
}

std::string encode_current_world() {
    Buffer buffer;
    TContext ctx{};
    Serializer ser{ctx, buffer};

    uint32_t version = 1;
    ser.value4b(version);

    const auto& ents = EntityHelper::get_entities();
    uint32_t count = 0;
    for (const auto& sp : ents)
        if (sp) ++count;
    ser.value4b(count);

    for (const auto& sp : ents) {
        if (!sp) continue;
        write_entity(ser, *sp);
    }

    ser.adapter().flush();
    return buffer;
}

bool decode_into_current_world(const std::string& blob) {
    TContext ctx{};
    Deserializer des{ctx, blob.begin(), blob.size()};

    uint32_t version = 0;
    des.value4b(version);
    if (des.adapter().error() != bitsery::ReaderError::NoError) return false;
    if (version != 1) return false;

    uint32_t num_entities = 0;
    des.value4b(num_entities);
    if (des.adapter().error() != bitsery::ReaderError::NoError) return false;

    Entities new_entities;
    new_entities.reserve(num_entities);
    for (uint32_t i = 0; i < num_entities; ++i) {
        std::shared_ptr<afterhours::Entity> sp(new afterhours::Entity());
        if (!read_entity(des, *sp)) return false;
        new_entities.push_back(std::move(sp));
    }

    EntityHelper::get_current_collection().replace_all_entities(
        std::move(new_entities));
    return des.adapter().error() == bitsery::ReaderError::NoError;
}

}  // namespace snapshot_blob

