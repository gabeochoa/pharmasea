

#pragma once

// Stop using GLFW directly with raylib 5.5; rely on raylib's monitor APIs

#include <algorithm>
#include <string>
#include <vector>

#include "../vendor_include.h"
#include "singleton.h"

namespace rez {
struct ResolutionInfo {
    int width;
    int height;

    bool operator<(const ResolutionInfo& r) const {
        return (this->width < r.width) ||
               ((this->width == r.width) && (this->height < r.height));
    }

    bool operator==(const ResolutionInfo& r) const {
        return (this->width == r.width) && (this->height == r.height);
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value4b(this->width);
        s.value4b(this->height);
    }
};

inline std::ostream& operator<<(std::ostream& os, const ResolutionInfo& info) {
    os << "Resolution: " << info.width << " x " << info.height << std::endl;
    return os;
}

SINGLETON_FWD(ResolutionExplorer)
struct ResolutionExplorer {
    SINGLETON(ResolutionExplorer)

    std::vector<ResolutionInfo> options;
    std::vector<std::string> options_str;

    [[nodiscard]] const ResolutionInfo& fetch(int index) const {
        return options[index];
    }

    void load_resolution_options() {
        options.clear();

        // Query current monitor via raylib and propose a set of common 16:9
        // modes
        int monitor = raylib::GetCurrentMonitor();
        int maxWidth = raylib::GetMonitorWidth(monitor);
        int maxHeight = raylib::GetMonitorHeight(monitor);

        const ResolutionInfo common[] = {
            {1280, 720},  {1366, 768},  {1600, 900},  {1920, 1080},
            {2048, 1152}, {2560, 1440}, {2880, 1620}, {3008, 1692},
            {3072, 1728}, {3200, 1800}, {3440, 1440}, {3840, 2160},
        };

        for (const auto& cand : common) {
            if (cand.height < 720 || cand.height > 2160) continue;
            if (cand.width > maxWidth || cand.height > maxHeight) continue;
            float aspect = cand.width / (cand.height * 1.f);
            if (std::fabs(1.7777f - aspect) > 0.12f) continue;
            options.push_back(cand);
        }

        if (options.empty()) {
            options.push_back(
                ResolutionInfo{.width = maxWidth, .height = maxHeight});
            options.push_back(ResolutionInfo{.width = 1920, .height = 1080});
            options.push_back(ResolutionInfo{.width = 1280, .height = 720});
        }

        // Deduplicate
        std::sort(options.begin(), options.end());
        options.erase(std::unique(options.begin(), options.end()),
                      options.end());
    }

    void convert_res_options_to_text() {
        std::transform(options.cbegin(), options.cend(),
                       std::back_inserter(options_str),
                       [](ResolutionInfo info) {
                           return fmt::format("{}x{}", info.width, info.height);
                       });
    }

    // Saving them is just an optimization so we dont have to fetch them all the
    // time it also means that the index is relatively stable at least once per
    // launch
    [[nodiscard]] std::vector<std::string> fetch_options() {
        if (options.empty()) load_resolution_options();
        if (options_str.empty()) convert_res_options_to_text();
        return options_str;
    }

    void add(ResolutionInfo info) { options.push_back(info); }

    [[nodiscard]] int index(ResolutionInfo info) {
        int i = 0;
        for (const auto res : options) {
            if (res == info) return i;
            i++;
        }
        return -1;
    }
};

}  // namespace rez
