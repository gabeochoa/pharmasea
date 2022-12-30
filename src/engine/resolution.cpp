
#include "resolution.h"

#include <algorithm>
//
#include <raylib/glfw3.h>

namespace rez {

void load_resolution_options() {
#ifdef __APPLE__
    // Nothing this one works :)
#else
    // TODO either implement these for windows or get them in the dll
    const auto glfwGetPrimaryMonitor = []() -> GLFWmonitor* { return nullptr; };
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

        RESOLUTION_OPTIONS.push_back(
            ResolutionInfo{.width = mode.width, .height = mode.height});
    }

    if (RESOLUTION_OPTIONS.empty()) {
        RESOLUTION_OPTIONS.push_back(
            ResolutionInfo{.width = 1280, .height = 720});
        RESOLUTION_OPTIONS.push_back(
            ResolutionInfo{.width = 1920, .height = 1080});
        RESOLUTION_OPTIONS.push_back(
            ResolutionInfo{.width = 3860, .height = 2160});
    }

    // TODO SPEED this kinda slow but it only happens once
    RESOLUTION_OPTIONS.erase(
        std::unique(RESOLUTION_OPTIONS.begin(), RESOLUTION_OPTIONS.end()),
        RESOLUTION_OPTIONS.end());
}

void convert_res_options_to_text() {
    std::transform(RESOLUTION_OPTIONS.cbegin(), RESOLUTION_OPTIONS.cend(),
                   std::back_inserter(STRING_RESOLUTION_OPTIONS),
                   [](ResolutionInfo info) {
                       return fmt::format("{}x{}", info.width, info.height);
                   });
}

std::vector<std::string> resolution_options() {
    if (RESOLUTION_OPTIONS.empty()) load_resolution_options();
    if (STRING_RESOLUTION_OPTIONS.empty()) convert_res_options_to_text();
    return STRING_RESOLUTION_OPTIONS;
}
}  // namespace rez
