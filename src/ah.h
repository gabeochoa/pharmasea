
#include "bitsery_include.h"

// Include the full expected.hpp first to provide expected/unexpected
#include <expected.hpp>

#define AFTER_HOURS_REPLACE_LOGGING
#include "log/log.h"
#define ENABLE_AFTERHOURS_BITSERY_SERIALIZE
#define AFTER_HOURS_SYSTEM
// Prevent afterhours from including its expected.hpp by pretending
// std::expected exists
#define __cpp_lib_expected 1
namespace std {
using tl::expected;
using tl::unexpected;
}  // namespace std
#include "afterhours/ah.h"
#include "afterhours/src/bitset_utils.h"
#include "afterhours/src/library.h"

namespace afterhours {
template<typename T>
using Library = ::Library<T>;
}
