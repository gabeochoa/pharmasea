
#define AFTER_HOURS_REPLACE_LOGGING
#include "log/log.h"
#define AFTER_HOURS_SYSTEM
#define AFTER_HOURS_USE_RAYLIB
#include "afterhours/ah.h"
#include "afterhours/src/bitset_utils.h"
#include "afterhours/src/library.h"
#include "afterhours/src/plugins/window_manager.h"

namespace afterhours {
template<typename T>
using Library = ::Library<T>;
}
