#pragma once

// Central include point for zpp::bits serialization.
//
// This mirrors the intent of `src/bitsery_include.h`: keep heavy serialization
// includes localized so we don't explode compile times across the whole project.
//
// NOTE:
// `zpp_bits.h` is vendored and (currently) triggers `-Wshadow` diagnostics on
// clang. Our builds treat warnings as errors, so we locally suppress shadow
// warnings for this include only.

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#endif

#include "../vendor/zpp_bits/zpp_bits.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

