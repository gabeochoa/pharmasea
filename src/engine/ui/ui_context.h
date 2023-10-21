
#pragma once

#include "../font_sizer.h"
#include "../uuid.h"
#include "state.h"
#include "ui_input_manager.h"
#include "ui_render_textures.h"

namespace ui {

extern std::atomic_int UICONTEXT_ID;
struct UIContext;
extern std::shared_ptr<UIContext> _uicontext;
// TODO do we need both _uicontext and globalContext?
// if not we can switch to using the SINGLETON macro
// NOTE: Jan2-23, I tried but it seems to immediately crash on startup, what is
// using this?
extern UIContext* globalContext;

struct UIContext : public IUIContextInputManager,
                   public IUIContextRenderTextures,
                   public FontSizeCache {
    [[nodiscard]] inline static UIContext* create() { return new UIContext(); }
    [[nodiscard]] inline static UIContext& get() {
        if (globalContext) return *globalContext;
        if (!_uicontext) _uicontext.reset(UIContext::create());
        return *_uicontext;
    }

   public:
    // Scissor info
    bool should_immediate_draw = false;
    std::optional<Rectangle> scissor_box;

    void scissor_on(Rectangle r) {
        should_immediate_draw = true;
        scissor_box = r;
        raylib::BeginScissorMode((int) r.x, (int) r.y, (int) r.width,
                                 (int) r.height);
    }
    void scissor_off() {
        raylib::EndScissorMode();
        should_immediate_draw = false;
        scissor_box = {};
    }
    //

    int id = 0;

    UIContext() : id(UICONTEXT_ID++) { this->init(); }

    StateManager statemanager;

    virtual void init() override {
        IUIContextInputManager::init();
        FontSizeCache::init();
    }

    void cleanup() override {
        globalContext = nullptr;
        IUIContextInputManager::cleanup();
    }

    template<typename T>
    [[nodiscard]] std::shared_ptr<T> widget_init(const uuid& uuid) {
        std::shared_ptr<T> state = statemanager.getAndCreateIfNone<T>(uuid);
        if (state == nullptr) {
            log_error(
                "State for id ({}) of wrong type, expected {}. Check to "
                "make sure your id's are globally unique",
                std::string(uuid), type_name<T>());
        }
        return state;
    }

    template<typename T>
    [[nodiscard]] std::shared_ptr<T> get_widget_state(const uuid& uuid) {
        return statemanager.get_as<T>(uuid);
    }
};

[[nodiscard]] inline UIContext& get() { return UIContext::get(); }

}  // namespace ui
