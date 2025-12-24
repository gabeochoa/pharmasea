#include "layout_wfc.h"

#include <sstream>
#include <string>
#include <vector>

#include "../engine/globals.h"
#include "ascii_grid.h"
#include "wave_collapse.h"

namespace mapgen {

namespace {

static std::vector<std::string> wfc_lines_no_log(const wfc::WaveCollapse& wc) {
    bool add_extra_space = false;
    std::vector<std::string> lines;
    std::stringstream temp;

    // NOTE: WaveCollapse indexes grid_options as [x * rows + y] throughout.
    // The current config uses square grids (rows == cols), so keep this
    // convention here for consistency.
    for (int r = 0; r < wc.rows; r++) {
        for (size_t i = 0; i < wc.patterns[0].pat.size(); i++) {
            for (int c = 0; c < wc.cols; c++) {
                int idx = r * wc.rows + c;
                int bit = bitset_utils::get_first_enabled_bit(
                    wc.grid_options[(size_t) idx]);
                if (bit == -1) {
                    temp << std::string(wc.patterns[0].pat.size(), '.');
                } else if (wc.grid_options[(size_t) idx].count() > 1) {
                    temp << std::string(wc.patterns[0].pat.size(), '?');
                } else {
                    temp << (wc.patterns[(size_t) bit].pat)[i];
                }
                if (add_extra_space) temp << " ";
            }
            lines.push_back(std::string(temp.str()));
            temp.str(std::string());
        }
        if (add_extra_space) lines.push_back(" ");
    }
    return lines;
}

}  // namespace

std::vector<std::string> generate_layout_wfc(const std::string& seed,
                                             const GenerationContext& ctx,
                                             int attempt_index) {
    wfc::ensure_map_generation_info_loaded();

    unsigned int wfc_seed = static_cast<unsigned int>(
        hashString(fmt::format("{}:wfc_layout:{}", seed, attempt_index)));
    wfc::WaveCollapse wc(wfc_seed);
    wc.run();

    std::vector<std::string> lines = wfc_lines_no_log(wc);
    grid::normalize_dims(lines, ctx.rows, ctx.cols);
    grid::scrub_to_layout_only(lines);
    return lines;
}

}  // namespace mapgen

