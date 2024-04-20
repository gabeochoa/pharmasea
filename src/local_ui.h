
#pragma once

#include "engine/ui/ui.h"
#include "engine/ui/widget.h"

namespace ps {

enum struct Size { Small, Medium, Large };

namespace internal {

inline Rectangle make_size(const Rectangle r, const Size& size) {
    float w_multiplier;
    float h_multiplier;

    switch (size) {
        case Size::Small:
            w_multiplier = 0.05f;
            h_multiplier = 0.04f;
            break;
        case Size::Medium:
            w_multiplier = 0.11f;
            h_multiplier = 0.08f;
            break;
        case Size::Large:
            w_multiplier = 0.22f;
            h_multiplier = 0.16f;
            break;
    }

    float width = WIN_WF() * w_multiplier;
    float height = WIN_HF() * h_multiplier;

    return Rectangle{r.x, r.y, width, height};
}

}  // namespace internal

bool button(const ui::Widget& w, const TranslatableString& trstring,
            const Size& size = Size::Medium) {
    return ui::button(ui::Widget{internal::make_size(w.rect, size)}, trstring);
}

bool checkbox(const ui::Widget& w, const ui::CheckboxData& data,
              const Size& size = Size::Medium) {
    ui::Widget w2{internal::make_size(w.rect, size)};
    return ui::checkbox(w2, data);
}

}  // namespace ps
