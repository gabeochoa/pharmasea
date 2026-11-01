#pragma once

#include <optional>
#include <string>

#include "graphics.h"

namespace gltf_loader {

std::optional<raylib::Model> load_model(const std::string& filename,
                                        std::string& warn_out,
                                        std::string& err_out);

}  // namespace gltf_loader
