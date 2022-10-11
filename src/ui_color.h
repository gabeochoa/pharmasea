
#pragma once

#include "external_include.h"

namespace ui {
namespace color {
static const Color white = Color{255, 255, 255, 255};
static const Color black = Color{0, 0, 0, 255};
static const Color red = Color{255, 0, 0, 255};
static const Color green = Color{0, 255, 0, 255};
static const Color blue = Color{0, 0, 255, 255};
static const Color teal = Color{0, 255, 255, 255};
static const Color magenta = Color{255, 0, 255, 255};

inline Color getOppositeColor(const Color& color) {
    return Color{                                            //
                 static_cast<unsigned char>(255 - color.r),  //
                 static_cast<unsigned char>(255 - color.g),  //
                 static_cast<unsigned char>(255 - color.b),  //
                 color.a};
}

}  // namespace color
}  // namespace ui
