
#pragma once

#include "external_include.h"
//
#include "assert.h"
#include "event.h"
#include "keymap.h"
#include "menu.h"
#include "raylib.h"
#include "vec_util.h"
//
#include "ui_autolayout.h"
#include "ui_state.h"
#include "ui_theme.h"
#include "ui_widget.h"
#include "uuid.h"

// TODO is there a better way to do this?
//
// Because we want the UI to be as simple as possible to write,
// devs dont have to specify the font size since its dynamic based on size.
//
// This means that like size, its caculated every frame. In case this causes
// any rendering lag**, we need some way of caching the font sizes given a
// certiain (content + width + height)
//
// ** I never actually noticed any frame drop so this optimization was premature
//
// ****************************************************** start kludge
namespace ui {

struct FZInfo {
    const std::string content;
    float width;
    float height;
    float spacing;
};
}  // namespace ui

namespace std {

template <class T>
inline void hash_combine(std::size_t & s, const T & v)
{
  std::hash<T> h;
  s^= h(v) + 0x9e3779b9 + (s<< 6) + (s>> 2);
}

template<>
struct std::hash<ui::FZInfo> {
    std::size_t operator()(const ui::FZInfo& info) const {
        using std::hash;
        using std::size_t;
        using std::string;

        std::size_t out = 0;
        hash_combine(out, info.content);
        hash_combine(out, info.width);
        hash_combine(out, info.height);
        hash_combine(out, info.spacing);
        return out;
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

   private:
    std::vector<std::shared_ptr<Widget>> temp_widgets;

   public:
    StateManager statemanager;
    std::stack<UITheme> themestack;
    Font font;
    Widget* root;
    std::stack<Widget*> parentstack;

    struct LastFrame {
        bool was_written_this_frame = false;
        std::optional<Rectangle> rect;
    };
    std::map<uuid, LastFrame> last_frame;
    LastFrame get_last_frame(uuid id) {
        if (!last_frame.contains(id)) {
            return LastFrame();
        }
        return last_frame[id];
    }
    void write_last_frame(const uuid& id, LastFrame f) {
        f.was_written_this_frame = true;
        last_frame[id] = f;
    }

    void reset_last_frame() {
        // Cleanup any things that were not written to
        // this frame
        auto it = last_frame.begin();
        while (it != last_frame.end()) {
            if (!(*it).second.was_written_this_frame) {
                last_frame.erase(it);
                continue;
            }
            it++;
        }
        //
        for (auto& kv : last_frame) {
            kv.second.was_written_this_frame = false;
        }
    }

    uuid hot_id;
    uuid active_id;
    uuid kb_focus_id;
    uuid last_processed;

    bool lmouse_down = false;
    vec2 mouse = vec2{0.0, 0.0};

    int key = -1;
    int mod = -1;
    GamepadButton button;

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

    bool process_gamepad_button_event(GamepadButtonPressedEvent event) {
        GamepadButton code = event.button;
        if (!KeyMap::does_layer_map_contain_button(STATE, code)) {
            return false;
        }
        button = code;
        return true;
    }

    bool _pressedButtonWithoutEat(GamepadButton butt) const {
        if (butt == GAMEPAD_BUTTON_UNKNOWN) return false;
        return button == butt;
    }

    bool pressedButtonWithoutEat(std::string name) const {
        GamepadButton code = KeyMap::get_button(STATE, name);
        return _pressedWithoutEat(code);
    }

    void eatButton() { button = GAMEPAD_BUTTON_UNKNOWN; }

    bool pressed(std::string name) {
        int code = KeyMap::get_key_code(STATE, name);
        bool a = _pressedWithoutEat(code);
        if (a) {
            eatKey();
            return a;
        }

        GamepadButton butt = KeyMap::get_button(STATE, name);
        bool b = _pressedButtonWithoutEat(butt);
        if (b) {
            eatButton();
        }
        return b;
    }

    bool _pressedWithoutEat(int code) const {
        if (code == KEY_NULL) return false;
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

    void end(Widget* tree_root) {
        autolayout::process_widget(tree_root);
        // tree_root->print_tree();
        // exit(0);
        render_all();
        cleanup();
    }

    void cleanup() {
        began_and_not_ended = false;
        if (lmouse_down) {
            if (active_id == ROOT_ID) {
                active_id = FAKE_ID;
            }
        } else {
            active_id = ROOT_ID;
        }
        // button = GAMEPAD_BUTTON_UNKNOWN;
        //
        key = int();
        mod = int();
        // keychar = int();
        // modchar = int();
        globalContext = nullptr;
        temp_widgets.clear();
        reset_last_frame();
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
                           float height, float spacing) {
        int font_size = 1;
        int last_size = 1;
        vec2 size;

        // how big can we make the size in powers of 2,
        // before growing outside our bounds
        do {
            last_size = font_size;
            font_size <<= 1;
            size = MeasureTextEx(font, content.c_str(), font_size, spacing);
            // std::cout << "got " << size.x << ", " << size.y << " for "
            // << font_size << " and " << width << ", " << height
            // << " and last was: " << last_size << std::endl;
        } while (size.x <= width && size.y <= height);

        // return the last one that passed
        return last_size;
    }

    int get_font_size(const std::string& content, float width, float height,
                      float spacing) {
        FZInfo fzinfo =
            FZInfo{.content = content, .width = width, .height = height};
        if (!_font_size_memo.contains(fzinfo)) {
            _font_size_memo[fzinfo] =
                get_font_size_impl(content, width, height, spacing);
        } else {
            // std::cout << "found value in cache" << std::endl;
        }
        int result = _font_size_memo[fzinfo];
        // std::cout << "cache value " << result << std::endl;
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

    void _draw_text(Rectangle rect, const std::string& content,
                    theme::Usage color_usage) {
        float spacing = 0.f;
        float font_size =
            get_font_size(content, rect.width, rect.height, spacing);
        // std::cout << "selected font size: " << font_size << " for " << rect.x
        // << ", " << rect.y << ", " << rect.width << ", " << rect.height
        // << std::endl;
        DrawTextEx(font,                                   //
                   content.c_str(),                        //
                   {rect.x, rect.y},                       //
                   font_size, spacing,                     //
                   active_theme().from_usage(color_usage)  //
        );
    }

    void draw_text(Widget* widget, const std::string& content,
                   theme::Usage color_usage) {
        _draw_text(widget->rect, content, color_usage);
    }

    void schedule_draw_text(Widget* widget, const std::string& content,
                            theme::Usage color_usage) {
        get().schedule_render_call(std::bind(&UIContext::draw_text, this,
                                             widget, content, color_usage));
    }

    void draw_widget_old(vec2 pos, vec2 size, float, Color color, std::string) {
        Rectangle rect = {pos.x, pos.y, size.x, size.y};
        DrawRectangleRounded(rect, 0.15f, 4, color);
    }

    inline vec2 widget_center(vec2 position, vec2 size) {
        return position + (size / 2.f);
    }

    std::shared_ptr<Widget> make_temp_widget(Widget* widget) {
        std::shared_ptr<Widget> temp(widget);
        temp_widgets.push_back(temp);
        return temp;
    }
};

UIContext& get() { return UIContext::get(); }

}  // namespace ui
