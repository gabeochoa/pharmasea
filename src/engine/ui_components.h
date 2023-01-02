
#pragma once

#include "ui_context.h"
#include "ui_widget.h"

namespace ui {
namespace components {

// DEFAULT COMPONENTS
const SizeExpectation icon_button_x = {.mode = Pixels, .value = 75.f};
const SizeExpectation icon_button_y = {.mode = Pixels, .value = 25.f};
const SizeExpectation button_x = {.mode = Pixels, .value = 130.f};
const SizeExpectation button_y = {.mode = Pixels, .value = 50.f};
const SizeExpectation padd_x = {.mode = Pixels, .value = 120.f};
const SizeExpectation padd_y = {.mode = Pixels, .value = 25.f};

inline std::shared_ptr<Widget> mk_button(uuid id, SizeExpectation bx = button_x,
                                         SizeExpectation by = button_y) {
    return get().own(Widget(id, bx, by));
}

inline std::shared_ptr<Widget> mk_icon_button(uuid id) {
    return get().own(Widget(id, icon_button_x, icon_button_y));
}

inline std::shared_ptr<Widget> mk_padding(SizeExpectation s1,
                                          SizeExpectation s2) {
    return get().own(Widget(s1, s2));
}

inline std::shared_ptr<Widget> mk_but_pad() {
    return get().own(Widget(padd_x, padd_y));
}

inline std::shared_ptr<Widget> mk_root() {
    return get().own(
        Widget(Size_Px(WIN_WF(), 1.f), Size_Px(WIN_HF(), 1.f), GrowFlags::Row));
}

inline std::shared_ptr<Widget> mk_row() {
    return get().own(Widget({.mode = Children, .strictness = 1.f},
                            {.mode = Children, .strictness = 1.f}, Row));
}

inline std::shared_ptr<Widget> mk_column() {
    return get().own(Widget({.mode = Children, .strictness = 1.f},
                            {.mode = Children, .strictness = 1.f}, Column));
}

inline std::shared_ptr<Widget> mk_text_id(uuid id) {
    return get().own(Widget(id, Size_Px(275.f, 0.5f), Size_Px(50.f, 1.f)));
}

inline std::shared_ptr<Widget> mk_text() {
    return get().own(Widget(Size_Px(275.f, 0.5f), Size_Px(50.f, 1.f)));
}

}  // namespace components

}  // namespace ui
