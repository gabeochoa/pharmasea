#include "world_snapshot_blob.h"

#include <bitset>

#include "../components/all_components.h"
#include "../engine/log.h"
#include "../entity_helper.h"
#include "../zpp_bits_include.h"

namespace snapshot_blob {

namespace {
using Buffer = std::string;
using OutArchive = zpp::bits::out<Buffer>;
using InArchive = zpp::bits::in<const Buffer>;

inline void clear_all_components(afterhours::Entity& e) {
    e.componentSet.reset();
    for (auto& ptr : e.componentArray) ptr.reset();
}

struct ComponentSerde {
    bool (*has)(afterhours::Entity&) = nullptr;
    std::errc (*write)(OutArchive&, afterhours::Entity&) = nullptr;
    std::errc (*read)(InArchive&, afterhours::Entity&) = nullptr;
};

template<typename T>
bool serde_has(afterhours::Entity& e) {
    return e.has<T>();
}

template<typename T>
std::errc serde_write(OutArchive& out, afterhours::Entity& e) {
    auto& cmp = e.get<T>();
    return out(cmp);
}

template<typename T>
std::errc serde_read(InArchive& in, afterhours::Entity& e) {
    auto& cmp = e.addComponent<T>();
    return in(cmp);
}

static const auto& component_serdes() {
    constexpr size_t kNum = std::tuple_size_v<snapshot_blob::ComponentTypes>;
    static_assert(kNum <= 255, "component count must fit in uint8_t");

    static const std::array<ComponentSerde, kNum> kSerdes = [] {
        std::array<ComponentSerde, kNum> out{};
        [&]<size_t... Is>(std::index_sequence<Is...>) {
            ((out[Is] =
                  ComponentSerde{
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
constexpr size_t kSnapshotComponentMaskWords =
    (kSnapshotComponentCount + 63) / 64;

template<typename Archive>
std::errc serialize_snapshot_mask(Archive& archive,
                                  SnapshotComponentMask& mask) {
    // NOTE: We do NOT use bitsery::ext::StdBitset here because it relies on
    // std::bitset::to_ullong(), which throws for N > 64 when higher bits are
    // set.
    for (size_t word_i = 0; word_i < kSnapshotComponentMaskWords; ++word_i) {
        std::uint64_t word = 0;

        if constexpr (std::remove_cvref_t<Archive>::kind() ==
                      zpp::bits::kind::in) {
            // Reader
            if (auto result = archive(word); zpp::bits::failure(result)) {
                return result;
            }
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
            if (auto result = archive(word); zpp::bits::failure(result)) {
                return result;
            }
        }
    }
    return {};
}

std::errc write_entity(OutArchive& out, afterhours::Entity& e) {
    // Versioned entity record.
    if (auto result = out(          //
            kEntitySnapshotVersion  //
        );
        zpp::bits::failure(result)) {
        return result;
    }

    if (auto result = out(  //
            e.id,           //
            e.entity_type,  //
            e.tags,         //
            e.cleanup       //
        );
        zpp::bits::failure(result)) {
        return result;
    }

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

    if (auto result = serialize_snapshot_mask(out, present);
        zpp::bits::failure(result)) {
        return result;
    }

    const auto& serdes = component_serdes();
    std::errc err{};
    [&]<size_t... Is>(std::index_sequence<Is...>) {
        (([&] {
             if (err != std::errc{}) return;
             if (!present.test(Is)) return;
             if (!serdes[Is].write) return;
             err = serdes[Is].write(out, e);
         }()),
         ...);
    }(std::make_index_sequence<std::tuple_size_v<ComponentTypes>>{});

    return err;
}

[[nodiscard]] std::errc read_entity(InArchive& in, afterhours::Entity& e) {
    clear_all_components(e);

    // Versioned entity record.
    std::uint32_t ver = 0;
    if (auto result = in(  //
            ver            //
        );
        zpp::bits::failure(result)) {
        return result;
    }
    if (ver != kEntitySnapshotVersion) return std::errc::protocol_error;

    if (auto result = in(   //
            e.id,           //
            e.entity_type,  //
            e.tags,         //
            e.cleanup       //
        );
        zpp::bits::failure(result)) {
        return result;
    }

    SnapshotComponentMask present{};
    if (auto result = serialize_snapshot_mask(in, present);
        zpp::bits::failure(result)) {
        return result;
    }

    const auto& serdes = component_serdes();
    std::errc err{};
    [&]<size_t... Is>(std::index_sequence<Is...>) {
        (([&] {
             if (err != std::errc{}) return;
             if (!present.test(Is)) return;
             if (!serdes[Is].read) return;
             err = serdes[Is].read(in, e);
         }()),
         ...);
    }(std::make_index_sequence<std::tuple_size_v<ComponentTypes>>{});

    return err;
}

}  // namespace

std::string encode_entity(const afterhours::Entity& entity) {
    thread_local size_t last_reserve = 0;
    Buffer buffer;
    if (last_reserve > 0) buffer.reserve(last_reserve);
    OutArchive out{buffer};
    auto& e = const_cast<afterhours::Entity&>(entity);
    if (auto result = write_entity(out, e); zpp::bits::failure(result)) {
        return {};
    }
    last_reserve = buffer.size();
    return buffer;
}

bool decode_into_entity(afterhours::Entity& entity, const std::string& blob) {
    InArchive in{blob};
    return zpp::bits::success(read_entity(in, entity));
}

std::string encode_current_world() {
    thread_local size_t last_reserve = 0;
    Buffer buffer;
    if (last_reserve > 0) buffer.reserve(last_reserve);
    OutArchive out{buffer};

    uint32_t version = kWorldSnapshotVersion;
    if (auto result = out(  //
            version         //
        );
        zpp::bits::failure(result)) {
        return {};
    }

    const auto& ents = EntityHelper::get_entities();
    uint32_t count = 0;
    for (const auto& sp : ents)
        if (sp) ++count;
    if (auto result = out(  //
            count           //
        );
        zpp::bits::failure(result)) {
        return {};
    }

    for (const auto& sp : ents) {
        if (!sp) continue;
        if (auto result = write_entity(out, *sp); zpp::bits::failure(result)) {
            return {};
        }
    }

    last_reserve = buffer.size();
    return buffer;
}

bool decode_into_current_world(const std::string& blob) {
    InArchive in{blob};

    uint32_t version = 0;
    if (auto result = in(  //
            version        //
        );
        zpp::bits::failure(result)) {
        return false;
    }
    if (version != kWorldSnapshotVersion) return false;

    uint32_t num_entities = 0;
    if (auto result = in(  //
            num_entities   //
        );
        zpp::bits::failure(result)) {
        return false;
    }

    Entities new_entities;
    new_entities.reserve(num_entities);
    for (uint32_t i = 0; i < num_entities; ++i) {
        std::shared_ptr<afterhours::Entity> sp(new afterhours::Entity());
        if (zpp::bits::failure(read_entity(in, *sp))) return false;
        new_entities.push_back(std::move(sp));
    }

    EntityHelper::get_current_collection().replace_all_entities(
        std::move(new_entities));
    return true;
}

}  // namespace snapshot_blob
