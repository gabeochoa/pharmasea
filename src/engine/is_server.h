
#pragma once

#include <thread>
//
#include "thread_role.h"

[[nodiscard]] static bool is_server() { return thread_role::is_server(); }
