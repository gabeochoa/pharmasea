#include "world_snapshot_blob.h"

#include "../bitsery_include.h"
#include "../engine/log.h"
#include "../entity_helper.h"
#include "../network/polymorphic_components.h"  // includes all component types + bitsery wiring

#include <bitsery/ext/std_bitset.h>

#include <bitset>

namespace snapshot_blob {

namespace {
using Buffer = std::string;
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;
using TContext = std::tuple<>;
using Serializer = bitsery::Serializer<OutputAdapter, TContext>;
using Deserializer = bitsery::Deserializer<InputAdapter, TContext>;

inline void clear_all_components(afterhours::Entity& e) {
    e.componentSet.reset();
    for (auto& ptr : e.componentArray) ptr.reset();
}

struct ComponentSerde {
    bool (*has)(afterhours::Entity&) = nullptr;
    void (*write)(Serializer&, afterhours::Entity&) = nullptr;
    void (*read)(Deserializer&, afterhours::Entity&) = nullptr;
};

template<typename T>
bool serde_has(afterhours::Entity& e) {
    return e.has<T>();
}

template<typename T>
void serde_write(Serializer& s, afterhours::Entity& e) {
    auto& cmp = e.get<T>();
    s.object(cmp);
}

template<typename T>
void serde_read(Deserializer& d, afterhours::Entity& e) {
    auto& cmp = e.addComponent<T>();
    d.object(cmp);
}

static const auto& component_serdes() {
    constexpr size_t kNum =
        std::tuple_size_v<snapshot_blob::ComponentTypes>;
    static_assert(kNum <= 255, "component count must fit in uint8_t");

    static const std::array<ComponentSerde, kNum> kSerdes = [] {
        std::array<ComponentSerde, kNum> out{};
        [&]<size_t... Is>(std::index_sequence<Is...>) {
            ((out[Is] = ComponentSerde{
                  &serde_has<std::tuple_element_t<Is, ComponentTypes>>,
                  &serde_write<std::tuple_element_t<Is, ComponentTypes>>,
                  &serde_read<std::tuple_element_t<Is, ComponentTypes>>}),
             ...);
        }(std::make_index_sequence<kNum>{});
        return out;
    }();

    return kSerdes;
}

constexpr std::uint32_t kEntitySnapshotVersion = 3;
constexpr std::uint32_t kWorldSnapshotVersion = 3;

constexpr size_t kSnapshotComponentCount =
    std::tuple_size_v<snapshot_blob::ComponentTypes>;
using SnapshotComponentMask = std::bitset<kSnapshotComponentCount>;
constexpr size_t kSnapshotComponentMaskWords = (kSnapshotComponentCount + 63) / 64;

template<typename S>
void serialize_snapshot_mask(S& s, SnapshotComponentMask& mask) {
    // NOTE: We do NOT use bitsery::ext::StdBitset here because it relies on
    // std::bitset::to_ullong(), which throws for N > 64 when higher bits are set.
    for (size_t word_i = 0; word_i < kSnapshotComponentMaskWords; ++word_i) {
        std::uint64_t word = 0;

        if constexpr (requires { s.adapter().error(); }) {
            // Reader
            s.value8b(word);
            for (size_t bit = 0; bit < 64; ++bit) {
                const size_t idx = word_i * 64 + bit;
                if (idx >= kSnapshotComponentCount) break;
                const bool on = ((word >> bit) & 1ull) != 0ull;
                mask.set(idx, on);
            }
        } else {
            // Writer
            for (size_t bit = 0; bit < 64; ++bit) {
                const size_t idx = word_i * 64 + bit;
                if (idx >= kSnapshotComponentCount) break;
                if (mask.test(idx)) word |= (1ull << bit);
            }
            s.value8b(word);
        }
    }
}

void write_entity(Serializer& s, afterhours::Entity& e) {
    // Versioned entity record.
    s.value4b(kEntitySnapshotVersion);

    s.value4b(e.id);
    s.value4b(e.entity_type);
    s.ext(e.tags, bitsery::ext::StdBitset{});
    s.value1b(e.cleanup);

    // We serialize a bitset of which components are present, then serialize
    // component payloads in the stable `ComponentTypes` order.
    SnapshotComponentMask present{};
    [&]<size_t... Is>(std::index_sequence<Is...>) {
        (([&] {
             using T = std::tuple_element_t<Is, ComponentTypes>;
             if (e.has<T>()) {
                 present.set(Is);
             }
         }()),
         ...);
    }(std::make_index_sequence<std::tuple_size_v<ComponentTypes>>{});

    serialize_snapshot_mask(s, present);

    const auto& serdes = component_serdes();
    [&]<size_t... Is>(std::index_sequence<Is...>) {
        (([&] {
             if (!present.test(Is)) return;
             if (!serdes[Is].write) return;
             serdes[Is].write(s, e);
         }()),
         ...);
    }(std::make_index_sequence<std::tuple_size_v<ComponentTypes>>{});
}

[[nodiscard]] bool read_entity(Deserializer& d, afterhours::Entity& e) {
    clear_all_components(e);

    // Versioned entity record.
    std::uint32_t ver = 0;
    d.value4b(ver);
    if (d.adapter().error() != bitsery::ReaderError::NoError) return false;
    if (ver != kEntitySnapshotVersion) return false;

    d.value4b(e.id);
    d.value4b(e.entity_type);
    d.ext(e.tags, bitsery::ext::StdBitset{});
    d.value1b(e.cleanup);

    SnapshotComponentMask present{};
    serialize_snapshot_mask(d, present);
    if (d.adapter().error() != bitsery::ReaderError::NoError) return false;

    const auto& serdes = component_serdes();
    [&]<size_t... Is>(std::index_sequence<Is...>) {
        (([&] {
             if (!present.test(Is)) return;
             if (!serdes[Is].read) return;
             serdes[Is].read(d, e);
         }()),
         ...);
    }(std::make_index_sequence<std::tuple_size_v<ComponentTypes>>{});

    if (d.adapter().error() != bitsery::ReaderError::NoError) return false;

    return true;
}

}  // namespace

std::string encode_entity(const afterhours::Entity& entity) {
    thread_local size_t last_reserve = 0;
    Buffer buffer;
    if (last_reserve > 0) buffer.reserve(last_reserve);
    TContext ctx{};
    Serializer ser{ctx, buffer};
    auto& e = const_cast<afterhours::Entity&>(entity);
    write_entity(ser, e);
    ser.adapter().flush();
    last_reserve = buffer.size();
    return buffer;
}

bool decode_into_entity(afterhours::Entity& entity, const std::string& blob) {
    TContext ctx{};
    Deserializer des{ctx, blob.begin(), blob.size()};
    const bool ok = read_entity(des, entity);
    return ok && des.adapter().error() == bitsery::ReaderError::NoError;
}

std::string encode_current_world() {
    thread_local size_t last_reserve = 0;
    Buffer buffer;
    if (last_reserve > 0) buffer.reserve(last_reserve);
    TContext ctx{};
    Serializer ser{ctx, buffer};

    uint32_t version = kWorldSnapshotVersion;
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
    last_reserve = buffer.size();
    return buffer;
}

bool decode_into_current_world(const std::string& blob) {
    TContext ctx{};
    Deserializer des{ctx, blob.begin(), blob.size()};

    uint32_t version = 0;
    des.value4b(version);
    if (des.adapter().error() != bitsery::ReaderError::NoError) return false;
    if (version != kWorldSnapshotVersion) return false;

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

