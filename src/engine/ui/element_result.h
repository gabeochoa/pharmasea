
#pragma once

#include <string>
#include <variant>

#include "../keymap.h"

namespace ui {

struct ScrollWindowResult {
    Rectangle sv;
    int z_index;
};

struct ElementResult {
    // no explicit on purpose
    ElementResult(bool val) : result(val) {}
    ElementResult(bool val, bool d) : result(val), data(d) {}
    ElementResult(bool val, int d) : result(val), data(d) {}
    ElementResult(bool val, float d) : result(val), data(d) {}
    ElementResult(bool val, Rectangle d) : result(val), data(d) {}
    ElementResult(bool val, const std::string& d) : result(val), data(d) {}
    ElementResult(bool val, const AnyInput& d) : result(val), data(d) {}
    ElementResult(bool val, const ScrollWindowResult& d)
        : result(val), data(d) {}

    template<typename T>
    T as() const {
        return std::get<T>(data);
    }

    operator bool() const { return result; }

   private:
    bool result = false;
    std::variant<bool, int, float, std::string, AnyInput, Rectangle,
                 ScrollWindowResult>
        data = 0;
};

}  // namespace ui
