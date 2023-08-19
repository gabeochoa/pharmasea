
#pragma once

#include "../preload.h"
#include "../vec_util.h"
#include "assert.h"
#include "event.h"
#include "gamepad_axis_with_dir.h"
#include "keymap.h"
#include "log.h"
#include "raylib.h"
#include "statemanager.h"
#include "type_name.h"
//
#include "font_sizer.h"
#include "ui_autolayout.h"
#include "ui_state.h"
#include "ui_theme.h"
#include "ui_widget.h"
#include "uuid.h"

namespace ui {

const menu::State STATE = menu::State::UI;

struct IUIContextInputManager {
    virtual ~IUIContextInputManager() {}

    virtual void init() {
        mouse_info = MouseInfo();
        yscrolled = 0.f;
    }

    virtual void begin(float dt) {
        mouse_info = get_mouse_info();
        // TODO Should this be more like mousePos?
        yscrolled += ext::get_mouse_wheel_move();

        lastDt = dt;
    }

    virtual void cleanup() {
        // button = GAMEPAD_BUTTON_UNKNOWN;
        //
        key = int();
        mod = int();

        keychar = int();
        modchar = int();

        // Note: if you stop doing any keyhelds for a bit, then we
        // reset you so the next key press is faster
        if (keyHeldDownTimer == prevKeyHeldDownTimer) {
            keyHeldDownTimer = 0.f;
        }
        prevKeyHeldDownTimer = keyHeldDownTimer;
    }

    MouseInfo mouse_info;

    int key = -1;
    int mod = -1;
    GamepadButton button;
    GamepadAxisWithDir axis_info;
    int keychar = -1;
    int modchar = -1;
    float yscrolled;
    // TODO can we use timer stuff for this? we have tons of these around
    // the codebase
    float keyHeldDownTimer = 0.f;
    float prevKeyHeldDownTimer = 0.f;
    float keyHeldDownTimerReset = 0.15f;
    float lastDt;

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
        if (butt == raylib::GAMEPAD_BUTTON_UNKNOWN) return false;
        return button == butt;
    }

    [[nodiscard]] bool pressedButtonWithoutEat(const InputName& name) const {
        GamepadButton code = KeyMap::get_button(STATE, name);
        return _pressedWithoutEat(code);
    }

    void eatButton() { button = raylib::GAMEPAD_BUTTON_UNKNOWN; }

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

    void handleBadGamepadAxis(const KeyMapInputRequestError&, menu::State,
                              const InputName) {
        // TODO theres currently no valid inputs for axis on UI items so this is
        // all just firing constantly. log_warn("{}: No gamepad axis in {} for
        // {}", err, state, magic_enum::enum_name(name));
    }

    void eatAxis() { axis_info = {}; }

    [[nodiscard]] bool _pressedWithoutEat(int code) const {
        if (code == raylib::KEY_NULL) return false;
        return key == code || mod == code;
    }
    // TODO is there a better way to do eat(string)?
    [[nodiscard]] bool pressedWithoutEat(const InputName& name) const {
        int code = KeyMap::get_key_code(STATE, name);
        return _pressedWithoutEat(code);
    }

    void eatKey() { key = int(); }

    [[nodiscard]] bool is_held_down_debounced(const InputName& name) {
        const bool is_held = is_held_down(name);
        if (keyHeldDownTimer < keyHeldDownTimerReset) {
            keyHeldDownTimer += lastDt;
            return false;
        }
        keyHeldDownTimer = 0.f;
        return is_held;
    }

    [[nodiscard]] bool is_held_down(const InputName& name) {
        return (bool) KeyMap::is_event(STATE, name);
    }
};

struct IUIContextRenderTextures {
    std::vector<raylib::RenderTexture2D> render_textures;

    [[nodiscard]] int get_new_render_texture() {
        raylib::RenderTexture2D tex =
            raylib::LoadRenderTexture(WIN_W(), WIN_H());
        render_textures.push_back(tex);
        return (int) render_textures.size() - 1;
    }

    void turn_on_render_texture(int rt_index) {
        BeginTextureMode(render_textures[rt_index]);
    }

    void turn_off_texture_mode() { raylib::EndTextureMode(); }

    void draw_texture(int rt_id, Rectangle source, vec2 position) {
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
            raylib::UnloadRenderTexture(target);
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

static std::atomic_int UICONTEXT_ID = 0;
struct UIContext;
static std::shared_ptr<UIContext> _uicontext;
// TODO do we need both _uicontext and globalContext?
// if not we can switch to using the SINGLETON macro
// NOTE: Jan2-23, I tried but it seems to immediately crash on startup, what is
// using this?
static UIContext* globalContext;
struct UIContext : public IUIContextInputManager,
                   public IUIContextRenderTextures,
                   public IUIContextTheming,
                   public FontSizeCache {
    [[nodiscard]] inline static UIContext* create() { return new UIContext(); }
    [[nodiscard]] inline static UIContext& get() {
        if (globalContext) return *globalContext;
        if (!_uicontext) _uicontext.reset(UIContext::create());
        return *_uicontext;
    }

   public:
    int id = 0;

    UIContext() : id(UICONTEXT_ID++) { this->init(); }

    StateManager statemanager;

    virtual void init() override {
        IUIContextInputManager::init();
        IUIContextTheming::init();
        FontSizeCache::init();
    }

    void cleanup() override {
        globalContext = nullptr;
        IUIContextInputManager::cleanup();
    }

    template<typename T>
    [[nodiscard]] std::shared_ptr<T> widget_init(const uuid id) {
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
    [[nodiscard]] std::shared_ptr<T> get_widget_state(const uuid id) {
        return statemanager.get_as<T>(id);
    }
};

[[nodiscard]] inline UIContext& get() { return UIContext::get(); }

}  // namespace ui
