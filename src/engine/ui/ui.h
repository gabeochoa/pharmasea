
#pragma once

#include "color.h"
#include "element_result.h"
#include "focus.h"
#include "rect.h"
#include "theme.h"
#include "ui_context.h"
#include "widget.h"

namespace ui {

// TODO im not able to move this to cpp without breaking tabbing
inline void begin(std::shared_ptr<ui::UIContext> ui_context, float dt) {
    bool new_screen = context ? context->id != ui_context->id : true;
    context = ui_context;

    if (new_screen) {
        focus::reset();
    }

    focus::begin();
    //
    context->begin(dt);
}

void end();

ElementResult hoverable(const Widget& widget);

ElementResult div(const Widget& widget, Color c, bool rounded = false);
ElementResult div(const Widget& widget, theme::Usage theme,
                  bool rounded = false);
ElementResult colored_text(const Widget& widget,
                           const TranslatableString& content, Color c = WHITE);
ElementResult window(const Widget& widget);

ElementResult text(const Widget& widget, const TranslatableString& content,
                   ui::theme::Usage color_usage = ui::theme::Usage::Font,
                   bool draw_background = false

);

ElementResult scroll_window(const Widget& widget, Rectangle view,
                            std::function<void(ScrollWindowResult)> children);

ElementResult button(const Widget& widget,
                     const TranslatableString& content = NO_TRANSLATE(""),
                     bool background = true,
                     bool draw_background_when_hot = true);
ElementResult image(const Widget& widget, const std::string& texture_name);

ElementResult image_button(const Widget& widget,
                           const std::string& texture_name);

ElementResult checkbox(const Widget& widget, const CheckboxData& data = {});

ElementResult slider(const Widget& widget, const SliderData& data = {});

ElementResult dropdown(const Widget& widget, DropdownData data);
ElementResult control_input_field(const Widget& widget,
                                  const TextfieldData& data = {});

ElementResult textfield(const Widget& widget, const TextfieldData& data);

}  // namespace ui
