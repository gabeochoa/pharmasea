#pragma once

#include <cstdint>
#include <string>

namespace afterhours {
struct Entity;
}  // namespace afterhours

// Pointer-free snapshot encode/decode helpers.
//
// Goal: keep save-game + network payloads free of pointer-linking contexts by
// serializing an explicit snapshot surface into a byte blob.
namespace snapshot_blob {

// Hard caps to avoid pathological allocations from corrupt data.
constexpr std::uint32_t MaxWorldSnapshotBytes = 64u * 1024u * 1024u;  // 64 MiB
constexpr std::uint32_t MaxEntitySnapshotBytes = 2u * 1024u * 1024u;  // 2 MiB

// Serialize the current (thread-local) entity collection into a byte blob.
[[nodiscard]] std::string encode_current_world();

// Deserialize a world blob and replace the current (thread-local) entity list.
// Returns false on decode errors.
[[nodiscard]] bool decode_into_current_world(const std::string& blob);

// Serialize just one entity into a byte blob (used by unit tests).
[[nodiscard]] std::string encode_entity(const afterhours::Entity& entity);

// Deserialize an entity blob into an existing entity (clears components first).
// Returns false on decode errors.
[[nodiscard]] bool decode_into_entity(afterhours::Entity& entity,
                                      const std::string& blob);

}  // namespace snapshot_blob
