#include "pipeline.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "../engine/globals.h"
#include "playability_spec.h"
#include "ascii_grid.h"
#include "day1_required_placement.h"
#include "day1_validation.h"
#include "layout_simple.h"
#include "layout_wfc.h"

namespace mapgen {

namespace {

static std::string_view lstrip_prefix(std::string_view s,
                                      std::string_view prefix) {
    if (s.size() < prefix.size()) return s;
    if (s.substr(0, prefix.size()) != prefix) return s;
    return s.substr(prefix.size());
}

BarArchetype pick_archetype_from_seed(const std::string& seed) {
    size_t h = hashString(seed);
    int bucket = static_cast<int>(h % 100);
    if (bucket < 40) return BarArchetype::OpenHall;
    if (bucket < 75) return BarArchetype::MultiRoom;
    if (bucket < 90) return BarArchetype::BackRoom;
    return BarArchetype::LoopRing;
}

}  // namespace

GeneratedAscii generate_ascii(const std::string& seed,
                             const GenerationContext& context) {
    // Optional seed prefixes for selecting the layout provider without adding
    // new settings plumbing:
    // - "wfc:" forces WFC layout
    // - "simple:" forces simple layout
    LayoutSource layout_source = context.layout_source;
    std::string normalized_seed = seed;
    {
        std::string_view s = seed;
        if (lstrip_prefix(s, "wfc:") != s) {
            layout_source = LayoutSource::Wfc;
            normalized_seed = std::string(lstrip_prefix(s, "wfc:"));
        } else if (lstrip_prefix(s, "simple:") != s) {
            layout_source = LayoutSource::Simple;
            normalized_seed = std::string(lstrip_prefix(s, "simple:"));
        }
    }
    if (normalized_seed.empty()) normalized_seed = "seed";

    BarArchetype archetype = pick_archetype_from_seed(normalized_seed);

    const auto attempt_generate_with_layout = [&](LayoutSource src)
        -> std::optional<std::vector<std::string>> {
        for (int attempt = 0; attempt < playability::DEFAULT_REROLL_ATTEMPTS;
             attempt++) {
            std::vector<std::string> lines;
            if (src == LayoutSource::Wfc) {
                lines = generate_layout_wfc(normalized_seed, context, attempt);
            } else {
                lines = generate_layout_simple(normalized_seed, context, archetype,
                                               attempt);
            }

            grid::normalize_dims(lines, context.rows, context.cols);

            std::mt19937 rng(static_cast<unsigned int>(hashString(fmt::format(
                "{}:place:{}",
                normalized_seed,
                attempt))));

            if (!place_required_day1(lines, rng)) continue;
            if (!validate_day1_ascii_plus_routing(lines)) continue;
            return lines;
        }
        return std::nullopt;
    };

    std::optional<std::vector<std::string>> maybe_lines =
        attempt_generate_with_layout(layout_source);

    // Fallback: if WFC fails too often, try simple layout so gameplay always
    // gets a valid start-of-day map.
    if (!maybe_lines.has_value() && layout_source == LayoutSource::Wfc) {
        log_warn("[mapgen] WFC failed all attempts for seed {}, falling back to simple layout",
                 normalized_seed);
        maybe_lines = attempt_generate_with_layout(LayoutSource::Simple);
    }

    // Last-ditch: generate something and hope runtime repair/manual reroll is used.
    std::vector<std::string> lines;
    if (maybe_lines.has_value()) {
        lines = std::move(*maybe_lines);
    } else {
        lines = generate_layout_simple(normalized_seed, context, archetype, 0);
        grid::normalize_dims(lines, context.rows, context.cols);
        grid::ensure_single_origin(lines);
    }

    GeneratedAscii out;
    out.lines = std::move(lines);
    out.archetype = archetype;
    return out;
}

}  // namespace mapgen

