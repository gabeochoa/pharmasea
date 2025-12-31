#pragma once

#include "bitsery_include.h"
#include "entity.h"

// Provides concrete component type declarations + bitsery serialize support.
#include "network/polymorphic_components.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace snapshot_v2 {

// This is a compact, pointer-free encoding of "all component values we currently
// support", in a fixed order. It mirrors the transitional network/save shim
// ordering in `src/entity_component_serialization.h`.
//
// NOTE: This blob is an implementation detail to restore full component state
// over the network quickly. We can later migrate to per-component lists/DTOs.

inline constexpr std::size_t kMaxEntityComponentsBlobBytes = 4 * 1024 * 1024;  // 4 MiB

[[nodiscard]] std::vector<std::uint8_t> encode_components_blob(const Entity& e);
void decode_components_blob(Entity& e, const std::vector<std::uint8_t>& blob);

}  // namespace snapshot_v2

