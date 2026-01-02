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

struct ComponentSerde {
    std::uint16_t kind = 0;
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
                  snapshot_blob::component_kind_for_index(Is),
                  &serde_has<std::tuple_element_t<Is, ComponentTypes>>,
                  &serde_write<std::tuple_element_t<Is, ComponentTypes>>,
                  &serde_read<std::tuple_element_t<Is, ComponentTypes>>}),
             ...);
        }(std::make_index_sequence<kNum>{});
        return out;
    }();

    return kSerdes;
}

void write_entity(Serializer& s, afterhours::Entity& e) {
    s.value4b(e.id);
    s.value4b(e.entity_type);
    s.ext(e.tags, bitsery::ext::StdBitset{});
    s.value1b(e.cleanup);

    const auto& serdes = component_serdes();
    uint8_t count = 0;
    for (const auto& serde : serdes) {
        if (serde.has && serde.has(e)) ++count;
    }
    s.value1b(count);

    for (const auto& serde : serdes) {
        if (!serde.has || !serde.write) continue;
        if (!serde.has(e)) continue;
        write_kind(s, static_cast<ComponentKind>(serde.kind));
        serde.write(s, e);
    }
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
        const std::uint16_t raw = static_cast<std::uint16_t>(kind);
        if (raw == 0) {
            log_warn("snapshot_blob: invalid component kind {}", (int)raw);
            return false;
        }
        const auto& serdes = component_serdes();
        const size_t serde_idx = static_cast<size_t>(raw - 1);
        if (serde_idx >= serdes.size()) {
            log_warn("snapshot_blob: unknown component kind {}", (int)raw);
            return false;
        }
        const auto& serde = serdes[serde_idx];
        if (!serde.read) return false;
        serde.read(d, e);

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

