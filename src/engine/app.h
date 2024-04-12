
#pragma once

#include "event.h"
#include "files.h"
#include "layer.h"
#include "singleton.h"

struct AppSettings {
    int fps;
    int width;
    int height;
    const char* title;
    raylib::TraceLogLevel logLevel = raylib::LOG_ERROR;
};

struct App;
extern std::shared_ptr<App> App_single;

// SINGLETON_FWD(App)
struct App {
    //    SINGLETON_PARAM(App, AppSettings)

    static App* create(const AppSettings& pt) {
        if (!App_single) App_single.reset(new App(pt));
        return App_single.get();
    }
    [[nodiscard]] static App& get() { return *App_single; }

    bool running = false;
    LayerStack layerstack;
    int width;
    int height;

    int prev_width = -1;
    int prev_height = -1;

    // TODO create a render texture library?
    raylib::RenderTexture2D mainRT;

    explicit App(const AppSettings&);
    ~App();

    void pushLayer(Layer* layer);
    void pushOverlay(Layer* layer);
    void onEvent(Event& event);
    bool onWindowResize(WindowResizeEvent event);
    bool onWindowFullscreen(WindowFullscreenEvent& event);
    void processEvent(Event& e);
    void close();
    void run();
    void loop(float dt);

   private:
    void draw_all_to_texture(float dt);
    void render_to_screen();

    static void start_post_processing();
    static void end_post_processing();

    /*
    static void start_remove_invisible();
    static void end_remove_invisible();
    */
};
