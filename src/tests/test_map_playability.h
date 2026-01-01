#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "../engine/assert.h"
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

inline void test_map_playability() {
    test_playability_missing_origin();
    test_playability_disconnected_4_neighbor();
    test_playability_happy_path();
}

}  // namespace tests
