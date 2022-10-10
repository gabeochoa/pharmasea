
#pragma once

#include "assert.h"
#include "external_include.h"
#include "keymap.h"
#include "raylib.h"
#include "uuid.h"
#include "vec_util.h"

namespace ui {

namespace color {
static const Color white = Color{255};
static const Color black = Color{0};
static const Color red = Color{255, 0, 0, 255};
static const Color green = Color{0, 255, 0, 255};
static const Color blue = Color{0, 0, 255, 255};
static const Color teal = Color{0, 255, 255, 255};
static const Color magenta = Color{255, 0, 255, 255};
}  // namespace color

struct WidgetConfig {
    vec2 position;
    vec2 size;
    float rotation;
    std::string text;

    struct Theme {
        enum ColorType {
            FONT = 0,
            FG = 0,
            BG,
        };

        Color fontColor = color::magenta;
        Color backgroundColor = color::black;
        std::string texture = "TEXTURE";

        Color color(Theme::ColorType type = ColorType::BG) const {
            if (type == ColorType::FONT || type == ColorType::FG) {
                // TODO raylib color doesnt support negatives ...
                if (fontColor.r < 0)  // default
                    return color::black;
                return fontColor;
            }
            if (backgroundColor.r < 0)  // default
                return color::white;
            return backgroundColor;
        }
    } theme;
};

struct UIContext;
static std::shared_ptr<UIContext> _uicontext;
UIContext* globalContext;
struct UIContext {
    uuid hot_id;
    uuid active_id;
    uuid kb_focus_id;
    uuid last_processed;

    bool lmouse_down;
    vec2 mouse;

    inline static UIContext* create() { return new UIContext(); }
    inline static UIContext& get() {
        if (globalContext) return *globalContext;
        if (!_uicontext) _uicontext.reset(UIContext::create());
        return *_uicontext;
    }

    bool is_mouse_inside(const Rectangle& rect) {
        return mouse.x >= rect.x && mouse.x <= rect.x + rect.width &&
               mouse.y >= rect.y && mouse.y <= rect.y + rect.height;
    }

    // Press and Released
    bool pressed(int) {
        // TODO
        return false;
    }

    // is held down
    bool is_held_down(int) {
        // TODO
        return false;
    }

    void draw_widget(vec2 pos, vec2 size, float, Color color, std::string) {
        DrawRectangle(pos.x, pos.y,    //
                      size.x, size.y,  //
                      color);
    }

    inline vec2 widget_center(vec2 position, vec2 size) {
        return position + (size / 2.f);
    }

    //
    bool inited = false;
    bool began_and_not_ended = false;

    void init() {
        inited = true;
        began_and_not_ended = false;

        hot_id = ROOT_ID;
        active_id = ROOT_ID;
        kb_focus_id = ROOT_ID;

        lmouse_down = false;
        mouse = vec2{};
    }

    void begin(bool mouseDown, const vec2& mousePos) {
        M_ASSERT(inited, "UIContext must be inited before you begin()");
        M_ASSERT(!began_and_not_ended,
                 "You should call end every frame before calling begin() "
                 "again ");
        began_and_not_ended = true;

        globalContext = this;
        hot_id = ROOT_ID;
        lmouse_down = mouseDown;
        mouse = mousePos;
    }

    void end() {
        began_and_not_ended = false;
        if (lmouse_down) {
            if (active_id == ROOT_ID) {
                active_id = FAKE_ID;
            }
        } else {
            active_id = ROOT_ID;
        }
        // key = int();
        // mod = int();
        //
        // keychar = int();
        // modchar = int();
        globalContext = nullptr;
    }
};

UIContext& get() { return UIContext::get(); }

inline bool is_hot(const uuid& id) { return (get().hot_id == id); }
inline bool is_active(const uuid& id) { return (get().active_id == id); }
inline bool is_active_or_hot(const uuid& id) {
    return is_hot(id) || is_active(id);
}
inline bool is_active_and_hot(const uuid& id) {
    return is_hot(id) && is_active(id);
}

inline void active_if_mouse_inside(const uuid id, const Rectangle& rect) {
    bool inside = get().is_mouse_inside(rect);
    if (inside) {
        get().hot_id = id;
        if (get().active_id == ROOT_ID && get().lmouse_down) {
            get().active_id = id;
        }
    }
    return;
}

inline void try_to_grab_kb(const uuid id) {
    if (get().kb_focus_id == ROOT_ID) {
        get().kb_focus_id = id;
    }
}

inline bool has_kb_focus(const uuid& id) { return (get().kb_focus_id == id); }
inline void draw_if_kb_focus(const uuid& id, std::function<void(void)> cb) {
    if (has_kb_focus(id)) cb();
}

inline void handle_tabbing(const uuid id) {
    bool next = KeyMap::is_event(Menu::State::UI, "Widget Next");
    bool mod = KeyMap::is_event(Menu::State::UI, "Widget Mod");

    if (has_kb_focus(id)) {
        if (get().pressed(next)) {
            get().kb_focus_id = ROOT_ID;
            if (get().is_held_down(mod)) {
                get().kb_focus_id = get().last_processed;
            }
        }
    }
    // before any returns
    get().last_processed = id;
}

bool _text_impl(const uuid id, const WidgetConfig& config) {
    // NOTE: currently id is only used for focus and hot/active,
    // we could potentially also track "selections"
    // with a range so the user can highlight text
    // not needed for supermarket but could be in the future?
    (void) id;
    // No need to render if text is empty
    if (config.text.empty()) return false;

    DrawText(config.text.c_str(), config.position.x, config.position.y,
             config.size.x,
             config.theme.color(WidgetConfig::Theme::ColorType::FONT));

    return true;
}

inline void _button_render(const uuid id, const WidgetConfig& config) {
    draw_if_kb_focus(id, [&]() {
        get().draw_widget(config.position, config.size + vec2{0.1f, 0.1f},
                          config.rotation, color::teal, "TEXTURE");
    });

    if (get().hot_id == id) {
        if (get().active_id == id) {
            get().draw_widget(config.position, config.size, config.rotation,
                              color::red, "TEXTURE");
        } else {
            // Hovered
            get().draw_widget(config.position, config.size, config.rotation,
                              color::green, "TEXTURE");
        }
    } else {
        get().draw_widget(config.position, config.size, config.rotation,
                          color::blue, "TEXTURE");
    }

    get().draw_widget(config.position, config.size, config.rotation,
                      config.theme.color(), config.theme.texture);

    if (config.text.size() != 0) {
        WidgetConfig textConfig(config);
        // TODO detect if the button color is dark
        // and change the color to white automatically
        // textConfig.theme.fontColor = getOppositeColor(config.theme.color());
        textConfig.position = config.position + vec2{config.size.x * 0.05f,
                                                     config.size.y * 0.25f};
        textConfig.size = vec2{config.size.y, config.size.y} * 0.75f;

        _text_impl(MK_UUID(id.ownerLayer, 0), textConfig);
    }
}

inline bool _button_pressed(const uuid id) {
    bool press = KeyMap::is_event(Menu::State::UI, "Widget Press");
    // check click
    if (has_kb_focus(id)) {
        if (get().pressed(press)) {
            return true;
        }
    }
    if (!get().lmouse_down && is_active_and_hot(id)) {
        get().kb_focus_id = id;
        return true;
    }
    return false;
}

//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////

bool text(const uuid id, const WidgetConfig& config) {
    return _text_impl(id, config);
}
bool button(const uuid id, WidgetConfig config) {
    // no state
    // config.position = get().widget_center(config.position, config.size);
    active_if_mouse_inside(id, Rectangle{config.position.x, config.position.y,
                                         config.size.x, config.size.y});
    try_to_grab_kb(id);
    _button_render(id, config);
    handle_tabbing(id);
    bool pressed = _button_pressed(id);
    return pressed;
}

}  // namespace ui
