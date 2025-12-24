#pragma once

#include <functional>
#include <optional>
#include <string>

namespace replay_validation {

struct Context {
    std::string replay_name;
};

using EndValidationFn = std::function<void(Context&)>;

struct Spec {
    std::string name;
    EndValidationFn on_end;
};

// Register a validation spec for a replay name (e.g. "completed_first_day").
void register_spec(const Spec& spec);

// Called by the replay system.
void start_replay(const std::string& replay_name);
void add_failure(const std::string& message);
void end_replay();

// Helpers
[[nodiscard]] bool enabled();
[[nodiscard]] std::optional<Spec> get_spec(const std::string& replay_name);

}  // namespace replay_validation

