

#pragma once

#include <string>
#include <vector>

#include "../vendor_include.h"

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

std::vector<std::string> resolution_options();

static std::vector<ResolutionInfo> RESOLUTION_OPTIONS;
static std::vector<std::string> STRING_RESOLUTION_OPTIONS;

}  // namespace rez
