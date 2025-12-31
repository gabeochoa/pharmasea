
#include "bitsery_include.h"

#define AFTER_HOURS_REPLACE_LOGGING
#include "log/log.h"
#define ENABLE_AFTERHOURS_BITSERY_SERIALIZE
#define AFTER_HOURS_SYSTEM
#include "afterhours/ah.h"
#include "afterhours/src/bitset_utils.h"
#include "afterhours/src/library.h"

namespace afterhours {
template<typename T>
using Library = ::Library<T>;
}
