#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "settings.h"

// Settings schema/config metadata for PSCFG (docs/settings_file_plan.md).
// This is the "format for specifying the code" so each value can carry a
// version lifecycle (since/deprecated/removed).
namespace settings_schema {

// PSCFG file-format/schema version. Bump when the on-disk contract changes.
constexpr int PSCFG_VERSION = 1;

enum class ValueType { Bool, I32, F32, Str, I32x2 };

// Lifecycle:
// - since: inclusive start (valid if file_version >= since)
// - deprecated_since: inclusive (warn but accept if file_version >= deprecated_since)
// - removed_in_version: exclusive end (invalid if file_version >= removed_in_version)
struct Lifecycle {
    int since = 1;
    std::optional<int> deprecated_since;
    std::optional<int> removed_in_version;
};

// Schema entry for one persisted setting.
//
// We store both:
// - name/type/lifecycle (validation + logging)
// - a mapping to the actual in-memory field(s) in settings::Data.
struct KeySpec {
    std::string_view section;
    std::string_view key;
    ValueType type;
    Lifecycle lifecycle;

    // Exactly one of these should be non-null depending on `type`.
    bool settings::Data::* bool_member = nullptr;
    float settings::Data::* f32_member = nullptr;
    std::string settings::Data::* str_member = nullptr;

    // For compound types (e.g. resolution i32x2), use explicit accessors.
    void (*get_i32x2)(const settings::Data&, int32_t& a, int32_t& b) = nullptr;
    void (*set_i32x2)(settings::Data&, int32_t a, int32_t b) = nullptr;
};

// Canonical ordering is the order returned by this list.
const std::vector<KeySpec>& all_keys();

// Returns nullptr if the key is unknown.
const KeySpec* find_key(std::string_view key);

}  // namespace settings_schema

