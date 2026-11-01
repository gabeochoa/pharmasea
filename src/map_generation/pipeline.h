#pragma once

#include <string>
#include <vector>

#include "../ah.h"

namespace mapgen {

enum class LayoutSource {
    Simple,
    Wfc,
};

enum class BarArchetype {
    OpenHall,
    MultiRoom,
    BackRoom,
    LoopRing,
};

struct GenerationContext {
    int rows = 20;
    int cols = 20;
    LayoutSource layout_source = LayoutSource::Simple;
};

struct GeneratedAscii {
    std::vector<std::string> lines;
    BarArchetype archetype = BarArchetype::OpenHall;
};

GeneratedAscii generate_ascii(const std::string& seed,
                              const GenerationContext& context);

}  // namespace mapgen
