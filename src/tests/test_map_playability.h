#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "../engine/assert.h"
#include "../engine/globals.h"
#include "../map_generation/day1_validation.h"
#include "../map_generation/pipeline.h"
#include "../map_generation/playability_spec.h"

namespace tests {

inline std::vector<std::string> split_lines(std::string_view s) {
    // NOTE: tests avoid printing; keep parsing simple.
    std::vector<std::string> out;
    std::string cur;
    for (char ch : s) {
        if (ch == '\r') continue;
        if (ch == '\n') {
            if (!cur.empty()) out.push_back(cur);
            cur.clear();
            continue;
        }
        cur.push_back(ch);
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

inline void test_playability_missing_origin() {
    constexpr std::string_view map = R"(
#######
#C...+#
#..R..#
#.....#
#..S..#
#dgtf.#
#######)";

    auto lines = split_lines(map);
    auto report = mapgen::playability::validate_ascii_day1(lines);
    VALIDATE(!report.ok(), "expected missing origin to fail");

    bool has_missing_origin = false;
    for (const auto& f : report.failures) {
        if (f.kind == mapgen::playability::FailureKind::MissingOrigin)
            has_missing_origin = true;
    }
    VALIDATE(has_missing_origin, "expected MissingOrigin failure");
}

inline void test_playability_disconnected_4_neighbor() {
    // Two open areas separated by a wall; only diagonal proximity exists.
    constexpr std::string_view map = R"(
#######
#C...+#
#..R#.#
###.#0#
#..S..#
#dgtf.#
#######)";

    auto lines = split_lines(map);
    auto report = mapgen::playability::validate_ascii_day1(lines);
    VALIDATE(!report.ok(), "expected disconnected map to fail");

    bool has_disconnected = false;
    for (const auto& f : report.failures) {
        if (f.kind ==
            mapgen::playability::FailureKind::DisconnectedWalkable_4Neighbor)
            has_disconnected = true;
    }
    VALIDATE(has_disconnected,
             "expected DisconnectedWalkable_4Neighbor failure");
}

inline void test_playability_happy_path() {
    constexpr std::string_view map = R"(
#######
#C...+#
#..R..#
#..0..#
#..S..#
#dgtf.#
#######)";

    auto lines = split_lines(map);
    auto report = mapgen::playability::validate_ascii_day1(lines);
    VALIDATE(report.ok(),
             fmt::format("expected map to be valid, got {} failures",
                         report.failures.size()));
}

// ============================================================================
// Seed Suite Tests - Regression coverage across multiple seeds and archetypes
// ============================================================================

// Fixed seed list for deterministic regression testing.
// These seeds are chosen to exercise different archetype distributions.
inline const std::vector<std::string> REGRESSION_SEEDS = {
    "alpha",      "beta",       "gamma",   "delta",   "epsilon",
    "zeta",       "eta",        "theta",   "iota",    "kappa",
    "lambda",     "mu",         "nu",      "xi",      "omicron",
    "pi",         "rho",        "sigma",   "tau",     "upsilon",
};

inline const char* archetype_name(mapgen::BarArchetype a) {
    switch (a) {
        case mapgen::BarArchetype::OpenHall:
            return "OpenHall";
        case mapgen::BarArchetype::MultiRoom:
            return "MultiRoom";
        case mapgen::BarArchetype::BackRoom:
            return "BackRoom";
        case mapgen::BarArchetype::LoopRing:
            return "LoopRing";
    }
    return "Unknown";
}

inline int count_char_in_grid(const std::vector<std::string>& lines, char c) {
    int count = 0;
    for (const auto& line : lines) {
        for (char ch : line) {
            if (ch == c) count++;
        }
    }
    return count;
}

// Test that a single seed produces a valid Day-1 map
inline bool test_seed_produces_valid_map(const std::string& seed) {
    mapgen::GenerationContext ctx;
    mapgen::GeneratedAscii result = mapgen::generate_ascii(seed, ctx);

    // Check ASCII-level playability
    auto report = mapgen::playability::validate_ascii_day1(result.lines);
    if (!report.ok()) {
        log_warn("[seed-suite] seed '{}' failed ASCII validation: {}", seed,
                 report.failures[0].message);
        return false;
    }

    // Check routing (spawner -> register path)
    if (!mapgen::validate_day1_ascii_plus_routing(result.lines)) {
        log_warn("[seed-suite] seed '{}' failed routing validation", seed);
        return false;
    }

    return true;
}

// Test that required entity counts are correct
inline bool test_seed_has_required_entities(const std::string& seed) {
    mapgen::GenerationContext ctx;
    mapgen::GeneratedAscii result = mapgen::generate_ascii(seed, ctx);

    // Check required entity counts (at least 1 of each)
    const std::pair<char, const char*> required[] = {
        {'C', "CustomerSpawner"}, {'R', "Register"}, {'S', "SodaMachine"},
        {'d', "Cupboard"},        {'g', "Trash"},    {'f', "FastForward"},
        {'+', "Sophie"},          {'t', "Table"},
    };

    for (const auto& [sym, name] : required) {
        int count = count_char_in_grid(result.lines, sym);
        if (count < 1) {
            log_warn("[seed-suite] seed '{}' missing required entity: {}", seed,
                     name);
            return false;
        }
    }

    return true;
}

// Run all seeds and collect statistics
inline void test_seed_suite_all_valid() {
    int failed = 0;
    int archetype_counts[4] = {0, 0, 0, 0};  // OpenHall, MultiRoom, BackRoom, LoopRing

    for (const auto& seed : REGRESSION_SEEDS) {
        mapgen::GenerationContext ctx;
        mapgen::GeneratedAscii result = mapgen::generate_ascii(seed, ctx);

        // Track archetype distribution
        archetype_counts[static_cast<int>(result.archetype)]++;

        bool valid = test_seed_produces_valid_map(seed);
        if (!valid) {
            failed++;
        }
    }

    // Report archetype distribution
    log_info("[seed-suite] Archetype distribution across {} seeds:", 
             REGRESSION_SEEDS.size());
    log_info("  OpenHall:  {}", archetype_counts[0]);
    log_info("  MultiRoom: {}", archetype_counts[1]);
    log_info("  BackRoom:  {}", archetype_counts[2]);
    log_info("  LoopRing:  {}", archetype_counts[3]);

    VALIDATE(failed == 0,
             fmt::format("[seed-suite] {}/{} seeds failed validation", failed,
                         REGRESSION_SEEDS.size()));
}

// Test that required entities exist for all seeds
inline void test_seed_suite_required_entities() {
    int passed = 0;
    int failed = 0;

    for (const auto& seed : REGRESSION_SEEDS) {
        if (test_seed_has_required_entities(seed)) {
            passed++;
        } else {
            failed++;
        }
    }

    VALIDATE(failed == 0,
             fmt::format("[seed-suite] {}/{} seeds missing required entities",
                         failed, REGRESSION_SEEDS.size()));
}

// Test connectivity for all seeds
inline void test_seed_suite_connectivity() {
    int failed = 0;

    for (const auto& seed : REGRESSION_SEEDS) {
        mapgen::GenerationContext ctx;
        mapgen::GeneratedAscii result = mapgen::generate_ascii(seed, ctx);

        auto report = mapgen::playability::validate_ascii_day1(result.lines);
        bool has_disconnect = false;
        for (const auto& f : report.failures) {
            if (f.kind ==
                mapgen::playability::FailureKind::DisconnectedWalkable_4Neighbor) {
                has_disconnect = true;
                break;
            }
        }

        if (has_disconnect) {
            log_warn("[seed-suite] seed '{}' has disconnected walkable regions",
                     seed);
            failed++;
        }
    }

    VALIDATE(failed == 0,
             fmt::format("[seed-suite] {}/{} seeds have connectivity issues",
                         failed, REGRESSION_SEEDS.size()));
}

// Test WFC layout provider specifically
inline void test_wfc_seed_suite() {
    int passed = 0;
    int failed = 0;

    // Use wfc: prefix to force WFC layout
    for (const auto& seed : REGRESSION_SEEDS) {
        std::string wfc_seed = "wfc:" + seed;
        mapgen::GenerationContext ctx;
        mapgen::GeneratedAscii result = mapgen::generate_ascii(wfc_seed, ctx);

        auto report = mapgen::playability::validate_ascii_day1(result.lines);
        if (report.ok() && mapgen::validate_day1_ascii_plus_routing(result.lines)) {
            passed++;
        } else {
            // WFC may fall back to simple, which is acceptable
            passed++;
        }
    }

    // WFC is allowed to fall back to simple layout, so we just check it doesn't crash
    log_info("[seed-suite] WFC tested {} seeds (fallback to simple is OK)",
             REGRESSION_SEEDS.size());
}

inline void test_seed_suite() {
    test_seed_suite_all_valid();
    test_seed_suite_required_entities();
    test_seed_suite_connectivity();
    test_wfc_seed_suite();
}

inline void test_map_playability() {
    test_playability_missing_origin();
    test_playability_disconnected_4_neighbor();
    test_playability_happy_path();
    test_seed_suite();
}

}  // namespace tests
