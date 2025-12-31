#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "settings.h"

namespace settings_pscfg {

// PSCFG v1 (see docs/settings_file_plan.md)
constexpr int CURRENT_SETTINGS_VERSION = 1;

struct Message {
    enum class Level { Warn, Error };

    Level level;
    int line;
    std::string text;
};

struct Assignment {
    std::string section;  // optional; may be empty
    std::string key;      // snake_case recommended
    bool starred = false;

    using Value = std::variant<bool, int32_t, float, std::string,
                               std::pair<int32_t, int32_t>>;
    Value value;

    int line = 1;
};

struct LoadResult {
    settings::Data data;
    bool used_defaults = true;
    std::vector<Message> messages;
};

// Parse + apply PSCFG settings overrides to defaults.
LoadResult load_from_string(std::string_view input,
                            const settings::Data& defaults,
                            int current_version = CURRENT_SETTINGS_VERSION);

// Write overrides-only PSCFG file contents.
std::string write_overrides_only(const settings::Data& current,
                                 const settings::Data& defaults,
                                 int current_version = CURRENT_SETTINGS_VERSION);

}  // namespace settings_pscfg

