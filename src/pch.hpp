#pragma once

// Standard library bundle used widely
#include "std_include.h"
#include <cmath>

// fmt (header-only usage)
#ifndef FMT_HEADER_ONLY
#define FMT_HEADER_ONLY
#endif
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/xchar.h>

// nlohmann json
#include <nlohmann/json.hpp>

// bitsery adapters/traits
#include "bitsery_include.h"

// magic_enum with widened range for raylib keycodes
#ifndef MAGIC_ENUM_RANGE_MAX
#define MAGIC_ENUM_RANGE_MAX 400
#endif
#include <magic_enum/magic_enum.hpp>

// argh CLI
#include <argh.h>

