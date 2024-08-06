

#pragma once

#include <array>

constexpr std::array<float, 4> MODEL_TEST_AREA = {100.f, -50.f, 30.f, 50.f};
constexpr std::array<float, 4> LOBBY_AREA = {25.f, 5.f, 15.f, 15.f};
constexpr std::array<float, 4> PROGRESSION_AREA = {14.f, -35.f, 20.f, 25.f};
constexpr std::array<float, 4> STORE_AREA = {-18.f, -35.f, 20.f, 25.f};
constexpr std::array<float, 4> BAR_AREA = {-25.f, -5.f, 30.f, 30.f};

namespace building {
constexpr std::array<float, 2> get_center(const std::array<float, 4>& area) {
    return std::array<float, 2>{area[0] + (area[2] / 2.f),
                                area[1] + (area[3] / 2.f)};
}

constexpr std::array<float, 2> get_min(const std::array<float, 4>& area) {
    return std::array<float, 2>{area[0], area[1]};
}

constexpr std::array<float, 2> get_max(const std::array<float, 4>& area) {
    return std::array<float, 2>{area[0] + area[2], area[1] + area[3]};
}
}  // namespace building
