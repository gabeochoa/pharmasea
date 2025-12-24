#include "replay_validation.h"

#include <sstream>
#include <unordered_map>
#include <vector>

#include "../../globals.h"
#include "../assert.h"

namespace replay_validation {
namespace {

struct State {
    std::string replay_name;
    std::vector<std::string> failures;
};

State& state() {
    static State s;
    return s;
}

std::unordered_map<std::string, Spec>& registry() {
    static std::unordered_map<std::string, Spec> r;
    return r;
}

}  // namespace

bool enabled() { return REPLAY_VALIDATE; }

void register_spec(const Spec& spec) { registry()[spec.name] = spec; }

std::optional<Spec> get_spec(const std::string& replay_name) {
    auto it = registry().find(replay_name);
    if (it == registry().end()) return std::nullopt;
    return it->second;
}

void start_replay(const std::string& replay_name) {
    if (!enabled()) return;
    state().replay_name = replay_name;
    state().failures.clear();
}

void add_failure(const std::string& message) {
    if (!enabled()) return;
    state().failures.push_back(message);
}

void end_replay() {
    if (!enabled()) return;

    Context ctx;
    ctx.replay_name = state().replay_name;

    auto spec_opt = get_spec(ctx.replay_name);
    if (!spec_opt.has_value()) {
        add_failure("No replay validation spec registered for '" +
                    ctx.replay_name + "'");
    } else if (spec_opt->on_end) {
        spec_opt->on_end(ctx);
    }

    if (!state().failures.empty()) {
        std::ostringstream oss;
        oss << "Replay validation failed for '" << ctx.replay_name << "':\n";
        for (const auto& f : state().failures) {
            oss << "- " << f << "\n";
        }
        VALIDATE(false, oss.str());
    }
}

}  // namespace replay_validation

