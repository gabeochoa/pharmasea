
#pragma once

#include "external_include.h"
//
#include "assert.h"
#include "event.h"
#include "keymap.h"
#include "menu.h"
#include "raylib.h"
#include "ui.h"
#include "vec_util.h"
//
#include "ui_autolayout.h"
#include "ui_state.h"
#include "ui_theme.h"
#include "ui_widget.h"
#include "uuid.h"

// TODO theres got to be a better way....
// ****************************************************** start kludge
namespace ui {
struct FZInfo {
    const std::string content;
    float width;
    float height;
};
}  // namespace ui

namespace std {

template<>
struct std::hash<ui::FZInfo> {
    std::size_t operator()(const ui::FZInfo& info) const {
        using std::hash;
        using std::size_t;
        using std::string;
        return ((hash<string>()(info.content) ^
                 (hash<float>()(info.width) << 1)) >>
                1) ^
               (hash<float>()(info.height) << 1);
    }
};

}  // namespace std

namespace ui {
bool operator==(const FZInfo& info, const FZInfo& other) {
    return std::hash<FZInfo>()(info) == std::hash<FZInfo>()(other);
}
}  // namespace ui

// ****************************************************** end kludge

namespace ui {
const Menu::State STATE = Menu::State::UI;

struct UIContext;
static std::shared_ptr<UIContext> _uicontext;
// TODO do we need both _uicontext and globalContext?
// if not we can switch to using the SINGLETON macro
UIContext* globalContext;
struct UIContext {
    inline static UIContext* create() { return new UIContext(); }
    inline static UIContext& get() {
        if (globalContext) return *globalContext;
        if (!_uicontext) _uicontext.reset(UIContext::create());
        return *_uicontext;
    }

    StateManager statemanager;
    std::stack<UITheme> themestack;
    Font font;
    Widget* root;
    std::stack<Widget*> parentstack;
    std::vector<std::shared_ptr<Widget>> temp_widgets;

    uuid last_processed;

    bool lmouse_down = false;
    vec2 mouse = vec2{0.0, 0.0};

    int key = -1;
    int mod = -1;

    bool is_mouse_inside(const Rectangle& rect) {
        return mouse.x >= rect.x && mouse.x <= rect.x + rect.width &&
               mouse.y >= rect.y && mouse.y <= rect.y + rect.height;
    }

    bool process_keyevent(KeyPressedEvent event) {
        int code = event.keycode;
        if (!KeyMap::does_layer_map_contain_key(STATE, code)) {
            return false;
        }
        // TODO make this a map if we have more
        if (code == KeyMap::get_key_code(STATE, "Widget Mod")) {
            mod = code;
            return true;
        }
        key = code;
        return true;
    }

    bool pressed(std::string name) {
        int code = KeyMap::get_key_code(STATE, name);
        bool a = _pressedWithoutEat(code);
        if (a) eatKey();
        return a;
    }
    bool _pressedWithoutEat(int code) const {
        return key == code || mod == code;
    }
    // TODO is there a better way to do eat(string)?
    bool pressedWithoutEat(std::string name) const {
        int code = KeyMap::get_key_code(STATE, name);
        return _pressedWithoutEat(code);
    }

    void eatKey() { key = int(); }

    // is held down
    bool is_held_down(std::string name) {
        // TODO
        return KeyMap::is_event(STATE, name);
    }

    inline void set_kb_focus(const uuid& id) {
        statemanager.reset_kb_focus();
        auto state = statemanager.get(id);
        state->has_kb_focus = true;
    }

    inline void set_active(const uuid& id) {
        statemanager.reset_active();
        auto state = statemanager.get(id);
        state->active = true;
    }

    inline void set_hot(const uuid& id) {
        statemanager.reset_active();
        auto state = statemanager.get(id);
        state->hot = true;
    }

    inline bool is_active(const uuid& id) {
        auto state = get().statemanager.get(id);
        return state->active;
    }

inline bool is_hot(const uuid& id) {
    auto state = get().statemanager.get(id);
    return state->hot;
}

    //
    bool inited = false;
    bool began_and_not_ended = false;

    void init() {
        inited = true;
        began_and_not_ended = false;

        auto root_state = widget_init<RootState>(ROOT_ID);
        auto fake_state = widget_init<FakeState>(FAKE_ID);

        set_hot(ROOT_ID);
        set_active(ROOT_ID);
        set_kb_focus(ROOT_ID);

        lmouse_down = false;
        mouse = vec2{};
    }

    void begin(bool mouseDown, const vec2& mousePos) {
        M_ASSERT(inited, "UIContext must be inited before you begin()");
        M_ASSERT(!began_and_not_ended,
                 "You should call end every frame before calling begin() "
                 "again ");
        began_and_not_ended = true;

        set_hot(ROOT_ID);

        lmouse_down = mouseDown;
        mouse = mousePos;

        globalContext = this;
    }

    void update_rects(Widget* cur){
        if(cur){
            if(cur->rect.x == -1){
                return;
            }
            statemanager.set_rect(cur->id, cur->rect);
            for(const auto& child : cur->children){
                update_rects(child);
            }
        }
    }

    void end(Widget* tree_root) {
        autolayout::process_widget(tree_root);
        update_rects(tree_root);

        // tree_root->print_tree();
        // exit(0);
        render_all();
        cleanup();
    }

    void cleanup() {
        began_and_not_ended = false;

        set_hot(ROOT_ID);
        set_active(ROOT_ID);
        set_kb_focus(ROOT_ID);

        if (lmouse_down) {
            if (is_active(ROOT_ID)) {
                set_active(FAKE_ID);
            }
        } else {
            set_active(ROOT_ID);
        }
        key = int();
        mod = int();
        // keychar = int();
        // modchar = int();
        globalContext = nullptr;
        temp_widgets.clear();
    }

    template<typename T>
    std::shared_ptr<T> widget_init(const uuid id) {
        std::shared_ptr<T> state = statemanager.getAndCreateIfNone<T>(id);
        if (state == nullptr) {
            // TODO add log support
            std::cout << "State for your id is of wrong type. Check to make "
                         "sure your ids are globally unique"
                      << std::endl;
            // log_error(
            // "State for id ({}) of wrong type, expected {}. Check to "
            // "make sure your id's are globally unique",
            // std::string(id), type_name<T>());
        }
        return state;
    }

    template<typename T>
    std::shared_ptr<T> get_widget_state(const uuid id) {
        return statemanager.get_as<T>(id);
    }

    void set_font(Font f) { font = f; }

    void push_theme(UITheme theme) { themestack.push(theme); }

    void pop_theme() { themestack.pop(); }

    UITheme active_theme() {
        if (themestack.empty()) return DEFAULT_THEME;
        return themestack.top();
    }

    void push_parent(Widget* widget) { parentstack.push(widget); }
    void pop_parent() { parentstack.pop(); }

    Widget* active_parent() {
        if (parentstack.empty()) {
            return nullptr;
        }
        return parentstack.top();
    }

    void add_child(Widget* child) { active_parent()->add_child(child); }

    void draw_widget(Widget widget, theme::Usage usage) {
        DrawRectangleRounded(widget.rect, 0.15f, 4,
                             active_theme().from_usage(usage));
    }

    std::unordered_map<FZInfo, int> _font_size_memo;

    int get_font_size_impl(const std::string& content, float width,
                           float height) {
        float spacing = 0.f;
        int font_size = 1;
        int last_size = 1;
        vec2 size;

        // how big can we make the size in powers of 2,
        // before growing outside our bounds
        do {
            last_size = font_size;
            font_size <<= 1;
            size = MeasureTextEx(font, content.c_str(), font_size, spacing);
        } while (size.x <= width && size.y <= height);

        // return the last one that passed
        return last_size;
    }

    int get_font_size(const std::string& content, float width, float height) {
        FZInfo fzinfo =
            FZInfo{.content = content, .width = width, .height = height};
        if (!_font_size_memo.contains(fzinfo)) {
            _font_size_memo[fzinfo] =
                get_font_size_impl(content, width, height);
        } else {
            // std::cout << "found value in cache" << std::endl;
        }
        int result = _font_size_memo[fzinfo];
        return result;
    }

    std::vector<std::function<void()>> queued_render_calls;
    void render_all() {
        for (auto render_call : queued_render_calls) {
            render_call();
        }
        queued_render_calls.clear();
    }
    void schedule_render_call(std::function<void()> cb) {
        queued_render_calls.push_back(cb);
    }

    void draw_text(Widget* widget, const std::string& content) {
        float spacing = 0.f;
        // TODO would there ever be more direction types? (reverse row? )
        float font_size =
            get_font_size(content, widget->rect.width, widget->rect.height);
        DrawTextEx(font,                                          //
                   content.c_str(),                               //
                   {widget->rect.x, widget->rect.y},              //
                   font_size, spacing,                            //
                   active_theme().from_usage(theme::Usage::Font)  //
        );
    }

    void schedule_draw_text(Widget* widget, const std::string& content) {
        get().schedule_render_call(
            std::bind(&UIContext::draw_text, this, widget, content));
    }

    void draw_text_old(const char* text, vec2 position, vec2 size) {
        DrawTextEx(font, text, position, size.x, 0,
                   active_theme().from_usage(theme::Usage::Font));
    }

    void draw_widget_old(vec2 pos, vec2 size, float, Color color, std::string) {
        Rectangle rect = {pos.x, pos.y, size.x, size.y};
        DrawRectangleRounded(rect, 0.15f, 4, color);
    }

    inline vec2 widget_center(vec2 position, vec2 size) {
        return position + (size / 2.f);
    }
};

UIContext& get() { return UIContext::get(); }

}  // namespace ui
