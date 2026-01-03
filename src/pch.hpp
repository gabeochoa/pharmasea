#pragma once

// Standard library bundle used widely
#include <cmath>

#include "std_include.h"

// fmt (header-only usage)
#ifndef FMT_HEADER_ONLY
#define FMT_HEADER_ONLY
#endif
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/xchar.h>

// afterhours helpers
#include "afterhours/src/bitset_utils.h"

// nlohmann json
#include <nlohmann/json.hpp>

// bitsery adapters/traits
#include "zpp_bits_include.h"

// magic_enum with widened range for raylib keycodes
#ifndef MAGIC_ENUM_RANGE_MAX
#define MAGIC_ENUM_RANGE_MAX 400
#endif
#include <magic_enum/magic_enum.hpp>

// argh CLI
#include <argh.h>

// bring logging macros early so vendor afterhours sees replacements
#ifndef AFTER_HOURS_REPLACE_LOGGING
#define AFTER_HOURS_REPLACE_LOGGING
#endif
#ifndef AFTER_HOURS_REPLACE_VALIDATE
#define AFTER_HOURS_REPLACE_VALIDATE
#endif
#include "log/log.h"
