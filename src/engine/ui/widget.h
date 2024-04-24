
#pragma once

#include "../../std_include.h"
#include "../../vec_util.h"
#include "../log.h"
#include "../uuid.h"
#include "raylib.h"

namespace ui {

// TODO add mode for Percent of window size?
enum SizeMode { Null, Pixels, Text, Percent, Children };
enum GrowFlags { None = (1 << 0), Row = (1 << 1), Column = (1 << 2) };

struct SizeExpectation {
    SizeMode mode = Pixels;
    // TODO is there a better default?
    float value = 100.f;
    float strictness = 0.5f;
};

#define Size_Px(v, s) \
    { .mode = Pixels, .value = v, .strictness = s }

#define Size_Pct(v, s) \
    { .mode = Percent, .value = v, .strictness = s }

#define Size_FullW(s) Size_Px(WIN_WF(), s)

#define Size_FullH(s) Size_Px(WIN_HF(), s)

inline std::ostream& operator<<(std::ostream& os, const SizeExpectation& exp) {
    switch (exp.mode) {
        case Null:
            os << "Null";
            break;
        case Pixels:
            os << exp.value << " pixels";
            break;
        case Text:
            os << "Size of given text";
            break;
        case Percent:
            os << exp.value * 100.f << " percent";
            break;
        case Children:
            os << "Width of all children";
            break;
        default:
            os << "Missing Case for " << exp.mode;
            break;
    }
    os << " @ ";
    os << round(exp.strictness * 100.f);
    os << "%)";
    return os;
}

enum struct TextfieldValidationDecisionFlag {
    // Nothing
    None = 0,
    // Stop any new forward input (ie max length)
    StopNewInput = 1 << 0,
    // Show checkmark / green text
    Valid = 1 << 1,
    // Show x / red text
    Invalid = 1 << 2,
};  // namespace ui

inline bool operator!(TextfieldValidationDecisionFlag f) {
    return f == TextfieldValidationDecisionFlag::None;
}

inline bool validation_test(TextfieldValidationDecisionFlag flag,
                            TextfieldValidationDecisionFlag mask) {
    return !!(flag & mask);
}

using TextFieldValidationFn =
    std::function<TextfieldValidationDecisionFlag(const std::string&)>;

inline float calculateScale(const vec2& rect_size, const vec2& image_size) {
    float scale_x = rect_size.x / image_size.x;
    float scale_y = rect_size.y / image_size.y;
    return std::min(scale_x, scale_y);
}

struct DropdownData {
    std::vector<std::string> options;
    int initial = 0;
};

struct CheckboxData {
    bool selected = false;
    std::string content;
    bool background = true;
};

struct TextfieldData {
    std::string content;
    TextFieldValidationFn validationFunction = {};
};

struct SliderData {
    bool vertical = false;
    float value = 0.f;
};

// TODO theoretically this should probably match the types that go into init<>
typedef std::variant<std::string, bool, float, CheckboxData, TextfieldData,
                     SliderData, DropdownData>
    InputDataSource;

static std::atomic_int WIDGET_ID = 0;

struct Widget {
    int id = 0;
    int z_index = 0;
    Rectangle rect = {0, 0, WIN_WF(), WIN_HF()};

    Widget(Rectangle r) : id(WIDGET_ID++), z_index(0), rect(r) {}
    Widget(Rectangle r, int z_) : id(WIDGET_ID++), z_index(z_), rect(r) {}

    Rectangle get_rect() const { return rect; }
};

}  // namespace ui
