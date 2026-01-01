#pragma once

#include <filesystem>

#include "../engine/assert.h"
#include "../engine/simulated_input/replay_validation.h"

namespace tests {

inline void test_replay_validation_smoke() {
    namespace fs = std::filesystem;

    // Ensure the recorded replay exists in-repo.
    const fs::path replay_path =
        fs::current_path() / "recorded_inputs" / "completed_first_day.txt";
    VALIDATE(fs::exists(replay_path),
             "expected recorded replay to exist: " + replay_path.string());

    // Ensure a matching validation spec is registered (registered on static init
    // by src/tests/replay_specs.cpp).
    auto spec = replay_validation::get_spec("completed_first_day");
    VALIDATE(spec.has_value(),
             "expected replay validation spec to exist for completed_first_day");
}

}  // namespace tests

