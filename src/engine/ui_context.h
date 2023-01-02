
#pragma once

#include "../menu.h"
#include "../preload.h"
#include "../vec_util.h"
#include "assert.h"
#include "event.h"
#include "gamepad_axis_with_dir.h"
#include "keymap.h"
#include "log.h"
#include "raylib.h"
#include "type_name.h"
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

template<class T>
inline void hash_combine(std::size_t& s, const T& v) {
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
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

struct IUIContextInputManager {
    virtual ~IUIContextInputManager() {}

    virtual void init() {
        mouse_info = MouseInfo();
        yscrolled = 0.f;
    }

    virtual void begin(float) {
        mouse_info = get_mouse_info();
        // TODO Should this be more like mousePos?
        yscrolled += GetMouseWheelMove();
    }

    virtual void cleanup() {
        // button = GAMEPAD_BUTTON_UNKNOWN;
        //
        key = int();
        mod = int();

        keychar = int();
        modchar = int();
    }

    MouseInfo mouse_info;

    int key = -1;
    int mod = -1;
    GamepadButton button;
    GamepadAxisWithDir axis_info;
    int keychar = -1;
    int modchar = -1;
    float yscrolled;

    [[nodiscard]] bool is_mouse_inside(const Rectangle& rect) const {
        auto mouse = mouse_info.pos;
        return mouse.x >= rect.x && mouse.x <= rect.x + rect.width &&
               mouse.y >= rect.y && mouse.y <= rect.y + rect.height;
    }

    [[nodiscard]] bool process_char_press_event(CharPressedEvent event) {
        keychar = event.keycode;
        return true;
    }

    [[nodiscard]] bool process_keyevent(KeyPressedEvent event) {
        int code = event.keycode;
        if (!KeyMap::does_layer_map_contain_key(STATE, code)) {
            return false;
        }
        // TODO make this a map if we have more
        if (code == KeyMap::get_key_code(STATE, InputName::WidgetMod)) {
            mod = code;
            return true;
        }
        if (code == KeyMap::get_key_code(STATE, InputName::WidgetCtrl)) {
            mod = code;
            return true;
        }

        // TODO same as above, but a separate map
        modchar = code;

        key = code;
        return true;
    }

    [[nodiscard]] bool process_gamepad_button_event(
        GamepadButtonPressedEvent event) {
        GamepadButton code = event.button;
        if (!KeyMap::does_layer_map_contain_button(STATE, code)) {
            return false;
        }
        button = code;
        return true;
    }

    [[nodiscard]] bool process_gamepad_axis_event(GamepadAxisMovedEvent event) {
        GamepadAxisWithDir info = event.data;
        if (!KeyMap::does_layer_map_contain_axis(STATE, info.axis)) {
            return false;
        }
        axis_info = info;
        return true;
    }

    [[nodiscard]] bool _pressedButtonWithoutEat(GamepadButton butt) const {
        if (butt == GAMEPAD_BUTTON_UNKNOWN) return false;
        return button == butt;
    }

    [[nodiscard]] bool pressedButtonWithoutEat(const InputName& name) const {
        GamepadButton code = KeyMap::get_button(STATE, name);
        return _pressedWithoutEat(code);
    }

    void eatButton() { button = GAMEPAD_BUTTON_UNKNOWN; }

    [[nodiscard]] bool pressed(const InputName& name) {
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
            return b;
        }

        bool c = KeyMap::get_axis(STATE, name)
                     .map([&](GamepadAxisWithDir axis) -> bool {
                         return axis_info.axis == axis.axis &&
                                ((axis.dir - axis_info.dir) >= EPSILON);
                     })
                     .map_error([&](auto exp) {
                         this->handleBadGamepadAxis(exp, STATE, name);
                     })
                     .value_or(false);
        if (c) {
            eatAxis();
        }
        return c;
    }

    void handleBadGamepadAxis(const KeyMapInputRequestError&, Menu::State,
                              const InputName) {
        // TODO theres currently no valid inputs for axis on UI items so this is
        // all just firing constantly. log_warn("{}: No gamepad axis in {} for
        // {}", err, state, magic_enum::enum_name(name));
    }

    void eatAxis() { axis_info = {}; }

    [[nodiscard]] bool _pressedWithoutEat(int code) const {
        if (code == KEY_NULL) return false;
        return key == code || mod == code;
    }
    // TODO is there a better way to do eat(string)?
    [[nodiscard]] bool pressedWithoutEat(const InputName& name) const {
        int code = KeyMap::get_key_code(STATE, name);
        return _pressedWithoutEat(code);
    }

    void eatKey() { key = int(); }

    // is held down
    [[nodiscard]] bool is_held_down(const InputName& name) {
        return (bool) KeyMap::is_event(STATE, name);
    }
};

struct IUIContextLastFrame {
    struct LastFrame {
        bool was_written_this_frame = false;
        std::optional<Rectangle> rect;
    };

    std::map<uuid, LastFrame> last_frame;

    [[nodiscard]] LastFrame get_last_frame(uuid id) const {
        if (!last_frame.contains(id)) {
            return LastFrame();
        }
        return last_frame.at(id);
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
            if (!(it->second.was_written_this_frame)) {
                // note: this must be post increment
                last_frame.erase(it++);
                continue;
            }
            it++;
        }
        //
        for (auto& kv : last_frame) {
            kv.second.was_written_this_frame = false;
        }
    }
};

struct IUIContextFont {
    Font font;
    std::unordered_map<FZInfo, float> _font_size_memo;

    virtual void init() { this->set_font(Preload::get().font); }

    void set_font(Font f) { font = f; }

    [[nodiscard]] float get_font_size_impl(const std::string& content,
                                           float width, float height,
                                           float spacing) const {
        float font_size = 1.0f;
        float last_size = 1.0f;
        vec2 size;

        // NOTE: if you are looking at a way to speed this up switch to using
        // powers of two and theres no need to ceil or cast to int. also
        // shifting ( font_size <<= 1) is a clean way to do this
        //

        // lets see how big can we make the size before growing
        // outside our bounds
        do {
            last_size = font_size;
            // the smaller the number we multiply by (>1) the better fitting the
            // text will be
            font_size = ceilf(font_size * 1.15f);
            log_trace("measuring for {}", font_size);
            size = MeasureTextEx(font, content.c_str(), font_size, spacing);
            log_trace("got {},{} for {} and {},{} and last was: {}", size.x,
                      size.y, font_size, width, height, last_size);
        } while (size.x <= width && size.y <= height);

        // return the last one that passed
        return last_size;
    }

    [[nodiscard]] float get_font_size(const std::string& content, float width,
                                      float height, float spacing) {
        FZInfo fzinfo =
            FZInfo{.content = content, .width = width, .height = height};
        if (!_font_size_memo.contains(fzinfo)) {
            _font_size_memo[fzinfo] =
                get_font_size_impl(content, width, height, spacing);
        } else {
            log_trace("found value in cache");
        }
        float result = _font_size_memo[fzinfo];
        log_trace("cache value was {}", result);
        return result;
    }
};

struct IUIContextRenderTextures {
    std::vector<RenderTexture2D> render_textures;

    [[nodiscard]] int get_new_render_texture() {
        RenderTexture2D tex = LoadRenderTexture(WIN_W(), WIN_H());
        render_textures.push_back(tex);
        return (int) render_textures.size() - 1;
    }

    void turn_on_render_texture(int rt_index) {
        BeginTextureMode(render_textures[rt_index]);
    }

    void turn_off_texture_mode() { EndTextureMode(); }

    void draw_texture(int rt_id, Rectangle source, Vector2 position) {
        auto target = render_textures[rt_id];
        DrawTextureRec(target.texture, source, position, WHITE);
        // (Rectangle){0, 0, (float) target.texture.width,
        // (float) -target.texture.height},
        // (Vector2){0, 0}, WHITE);
    }

    void unload_all_render_textures() {
        // Note: we have to do this here because we cant unload until the entire
        // frame is written to the screen. We can guaranteed its definitely
        // rendered by the time it reaches the begin for the next frame
        for (auto target : render_textures) {
            UnloadRenderTexture(target);
        }
        render_textures.clear();
    }
};

struct IUIContextTheming {
    std::stack<UITheme> themestack;

    virtual void init() { this->push_theme(ui::DEFAULT_THEME); }

    void push_theme(UITheme theme) { themestack.push(theme); }

    void pop_theme() { themestack.pop(); }

    [[nodiscard]] UITheme active_theme() const {
        if (themestack.empty()) return DEFAULT_THEME;
        return themestack.top();
    }
};

struct IUIContextParentStacking {
    std::stack<Widget*> parentstack;

    void push_parent(Widget* widget) { parentstack.push(widget); }
    void pop_parent() { parentstack.pop(); }

    [[nodiscard]] Widget* active_parent() {
        if (parentstack.empty()) {
            return nullptr;
        }
        return parentstack.top();
    }

    void push_parent(std::shared_ptr<Widget> widget) {
        push_parent(widget.get());
    }
    void add_child(Widget* child) { active_parent()->add_child(child); }
};

struct UIContext;
static std::shared_ptr<UIContext> _uicontext;
// TODO do we need both _uicontext and globalContext?
// if not we can switch to using the SINGLETON macro
// NOTE: Jan2-23, I tried but it seems to immediately crash on startup, what is
// using this?
UIContext* globalContext;
struct UIContext : public IUIContextInputManager,
                   public IUIContextLastFrame,
                   public IUIContextFont,
                   public IUIContextRenderTextures,
                   public IUIContextTheming,
                   public IUIContextParentStacking {
    inline static UIContext* create() { return new UIContext(); }
    inline static UIContext& get() {
        if (globalContext) return *globalContext;
        if (!_uicontext) _uicontext.reset(UIContext::create());
        return *_uicontext;
    }

   private:
    std::vector<std::shared_ptr<Widget>> temp_widgets;

   public:
    UIContext() { this->init(); }

    StateManager statemanager;
    std::vector<std::shared_ptr<Widget>> internal_roots;
    std::vector<std::shared_ptr<Widget>> owned_widgets;

    std::shared_ptr<Widget> own(const Widget& widget) {
        owned_widgets.push_back(std::make_shared<Widget>(widget));
        return *owned_widgets.rbegin();
    }

    uuid hot_id;
    uuid active_id;
    uuid kb_focus_id;
    uuid last_processed;

    bool began_and_not_ended = false;

    virtual void init() override {
        began_and_not_ended = false;

        hot_id = ROOT_ID;
        active_id = ROOT_ID;
        kb_focus_id = ROOT_ID;

        IUIContextInputManager::init();
        IUIContextTheming::init();
        IUIContextFont::init();
    }

    virtual void begin(float dt) override {
        M_ASSERT(!began_and_not_ended,
                 "You should call end() every frame before calling begin() "
                 "again ");
        began_and_not_ended = true;

        IUIContextInputManager::begin(dt);

        globalContext = this;
        hot_id = ROOT_ID;

        unload_all_render_textures();
    }

    void end(Widget* tree_root) {
        autolayout::process_widget(tree_root);

        // tree_root->print_tree();
        // exit(0);

        render_all();
        reset_tabbing_if_not_visible(tree_root);
        //
        cleanup();
    }

    [[nodiscard]] bool matching_id_in_tree(uuid& id, Widget* tree_root) {
        if (tree_root == nullptr) return false;
        if (tree_root->id == id) return true;

        for (auto child : tree_root->children) {
            bool matching = matching_id_in_tree(id, child);
            if (matching) return true;
        }
        return false;
    }

    void reset_tabbing_if_not_visible(Widget* tree_root) {
        if (kb_focus_id == ROOT_ID) return;
        if (matching_id_in_tree(kb_focus_id, tree_root)) return;
        kb_focus_id = ROOT_ID;
    }

    void cleanup() override {
        began_and_not_ended = false;

        globalContext = nullptr;
        temp_widgets.clear();
        owned_widgets.clear();
        reset_last_frame();

        IUIContextInputManager::cleanup();
        // TODO is there some way to move this t input manager?
        if (mouse_info.leftDown) {
            if (active_id == ROOT_ID) {
                active_id = FAKE_ID;
            }
        } else {
            active_id = ROOT_ID;
        }
    }

    template<typename T>
    std::shared_ptr<T> widget_init(const uuid id) {
        std::shared_ptr<T> state = statemanager.getAndCreateIfNone<T>(id);
        if (state == nullptr) {
            log_error(
                "State for id ({}) of wrong type, expected {}. Check to "
                "make sure your id's are globally unique",
                std::string(id), type_name<T>());
        }
        return state;
    }

    template<typename T>
    std::shared_ptr<T> get_widget_state(const uuid id) {
        return statemanager.get_as<T>(id);
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
        log_trace("selected font size: {} for {}", font_size, rect);
        DrawTextEx(font,                                   //
                   content.c_str(),                        //
                   {rect.x, rect.y},                       //
                   font_size, spacing,                     //
                   active_theme().from_usage(color_usage)  //
        );
    }

    void draw_text(Widget* widget, const std::string& content,
                   theme::Usage color_usage) {
        if (!widget) return;
        _draw_text(widget->rect, content, color_usage);
    }

    void schedule_draw_text(Widget* widget, const std::string& content,
                            theme::Usage color_usage) {
        get().schedule_render_call(std::bind(&UIContext::draw_text, this,
                                             widget, content, color_usage));
    }

    void draw_widget_rect(Rectangle rect, theme::Usage usage) {
        DrawRectangleRounded(rect, 0.15f, 4, active_theme().from_usage(usage));
    }

    void draw_widget(Widget widget, theme::Usage usage) {
        draw_widget_rect(widget.rect, usage);
    }

    inline vec2 widget_center(vec2 position, vec2 size) const {
        return position + (size / 2.f);
    }

    std::shared_ptr<Widget> make_temp_widget(Widget* widget) {
        std::shared_ptr<Widget> temp(widget);
        temp_widgets.push_back(temp);
        return temp;
    }

    void schedule_render_texture(int rt_id, Rectangle source,
                                 Vector2 position) {
        get().schedule_render_call(
            std::bind(&UIContext::draw_texture, this, rt_id, source, position));
    }
};

UIContext& get() { return UIContext::get(); }

}  // namespace ui
