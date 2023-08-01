#pragma once

#include <fstream>
#include <sstream>
#include <string>

// TODO eventually merge into the other utils

// TODO switch to the one in preload or something
inline std::string loadFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
