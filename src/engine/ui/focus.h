
#pragma once

namespace ui {

static std::shared_ptr<ui::UIContext> context;

namespace focus {
static const int ROOT_ID = -1;
static const int FAKE_ID = -2;
static int focus_id = ROOT_ID;
static int last_processed = ROOT_ID;

static int hot_id = ROOT_ID;
static int active_id = ROOT_ID;
static MouseInfo mouse_info;

static std::set<int> ids;

inline bool is_mouse_inside(const Rectangle& rect) {
    auto mouse = mouse_info.pos;
    return mouse.x >= rect.x && mouse.x <= rect.x + rect.width &&
           mouse.y >= rect.y && mouse.y <= rect.y + rect.height;
}

inline bool is_mouse_down_in_box(const Rectangle& rect) {
    return is_mouse_inside(rect) && mouse_info.leftDown;
}

inline int get() { return focus_id; }
inline void set(int id) { focus_id = id; }
inline bool matches(int id) { return get() == id; }
inline bool up_for_grabs() { return matches(ROOT_ID); }
inline void try_to_grab(const Widget& widget) {
    ids.insert(widget.id);
    if (up_for_grabs()) {
        set(widget.id);
    }
}
inline void set_previous() { focus_id = last_processed; }

inline void set_hot(int id) { hot_id = id; }
inline void set_active(int id) { active_id = id; }
inline bool is_hot(int id) { return hot_id == id; }
inline bool is_active(int id) { return active_id == id; }
inline bool is_active_or_hot(int id) { return is_hot(id) || is_active(id); }
inline bool is_active_and_hot(int id) { return is_hot(id) && is_active(id); }
inline bool was_last(int id) { return id == last_processed; }

inline void active_if_mouse_inside(const Widget& widget,
                                   std::optional<Rectangle> view = {}) {
    bool inside =
        is_mouse_inside(view.has_value() ? view.value() : widget.get_rect());
    if (inside) {
        set_hot(widget.id);
        if (is_active(ROOT_ID) && mouse_info.leftDown) {
            set_active(widget.id);
        }
    }
}

inline bool is_mouse_click(const Widget& widget) {
    bool let_go_of_mouse = !mouse_info.leftDown;
    return let_go_of_mouse && is_active_and_hot(widget.id);
}

inline void handle_tabbing(const Widget& widget) {
    // TODO How do we handle something that wants to use
    // Widget Value Down/Up to control the value?
    // Do we mark the widget type with "nextable"? (tab will always work but
    // not very discoverable
    if (matches(widget.id)) {
        if (
            //
            context->pressed(InputName::WidgetNext) ||
            context->pressed(InputName::ValueDown)
            // TODO add support for holding down tab
            // get().is_held_down_debounced(InputName::WidgetNext) ||
            // get().is_held_down_debounced(InputName::ValueDown)
        ) {
            set(ROOT_ID);
            if (context->is_held_down(InputName::WidgetMod)) {
                set(last_processed);
            }
        }
        if (context->pressed(InputName::ValueUp)) {
            set(last_processed);
        }
        if (context->pressed(InputName::WidgetBack)) {
            set(last_processed);
        }
    }
    // before any returns
    last_processed = widget.id;
}

inline void reset() {
    focus_id = ROOT_ID;
    last_processed = ROOT_ID;

    hot_id = ROOT_ID;
    active_id = ROOT_ID;
    ids.clear();
}

inline void begin() {
    mouse_info = get_mouse_info();
    hot_id = ROOT_ID;
    WIDGET_ID = 0;
}

inline void end() {
    if (up_for_grabs()) return;

    if (mouse_info.leftDown) {
        if (is_active(ROOT_ID)) {
            set_active(FAKE_ID);
        }
    } else {
        set_active(ROOT_ID);
    }

    if (!ids.contains(focus_id)) {
        focus_id = ROOT_ID;
    }
    ids.clear();
}

}  // namespace focus
}  // namespace ui
