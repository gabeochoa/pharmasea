
#pragma once

#include "raylib.h"
//
#include "ui_autolayout.h"
#include "ui_color.h"
#include "ui_components.h"
#include "ui_context.h"
#include "ui_state.h"
#include "ui_theme.h"
#include "ui_widget.h"

//
#include "texture_library.h"
#include "uuid.h"

namespace ui {

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

typedef std::function<TextfieldValidationDecisionFlag(const std::string&)>
    TextFieldValidationFn;

// TODO add ability to have a cursor and move the cursor with arrow keys
bool textfield(
    // returns true if text changed
    const Widget& widget,
    // the string value being edited
    std::string& content,
    // validation function that returns a flag describing what the text
    // input should do
    TextFieldValidationFn validation = {});

bool checkbox(
    // Returns true if the checkbox changed
    const Widget& widget,
    // whether or not the checkbox is X'd
    bool* cbState = nullptr,
    // optional label to replace X and ``
    std::string* label = nullptr);

}  // namespace ui
