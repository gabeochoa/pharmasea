

#pragma once

#include <chrono>

namespace now {

inline long current_ms() {
    return (long) std::chrono::time_point_cast<std::chrono::milliseconds>(
               std::chrono::high_resolution_clock::now())
        .time_since_epoch()
        .count();
}

}  // namespace now
