

#pragma once

#include <raylib/glfw3.h>

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

    template<class Archive>
    void serialize(Archive& archive) {
        archive(width, height);
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
#ifdef __APPLE__
        // Nothing needed here this one works :)
#else
        // TODO either implement these for windows or get them in the dll
        const auto glfwGetPrimaryMonitor = []() -> GLFWmonitor* {
            return nullptr;
        };
        const auto glfwGetVideoModes = [](GLFWmonitor*, int*) -> GLFWvidmode* {
            return nullptr;
        };
#endif
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        int count = 0;
        const GLFWvidmode* modes = glfwGetVideoModes(monitor, &count);

        for (int i = 0; i < count; i++) {
            GLFWvidmode mode = modes[i];
            // int width
            // int height
            // int redBits int greenBits int blueBits
            // int refreshRate

            // Just kinda easier to not support every possible resolution
            if (mode.height < 720 || mode.height > 2160) continue;
            if (1.77f - (mode.width / (mode.height * 1.f)) > 0.1f) continue;

            options.push_back(
                ResolutionInfo{.width = mode.width, .height = mode.height});
        }

        if (options.empty()) {
            options.push_back(ResolutionInfo{.width = 1280, .height = 720});
            options.push_back(ResolutionInfo{.width = 1920, .height = 1080});
            options.push_back(ResolutionInfo{.width = 3860, .height = 2160});
        }

        // TODO SPEED this kinda slow but it only happens once
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
